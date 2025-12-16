// Fill out your copyright notice in the Description page of Project Settings.


#include "Synergy/PCSynergyBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "BaseGameplayTags.h"
#include "AbilitySystem/Unit/EffectSpec/PCEffectSpec.h"
#include "Character/Unit/PCHeroUnitCharacter.h"
#include "DataAsset/Unit/PCDataAsset_UnitAbilityConfig.h"
#include "DataAsset/Synergy/PCDataAsset_SynergyData.h"
#include "GameFramework/GameState/PCCombatGameState.h"
#include "Particles/ParticleSystem.h"


void UPCSynergyBase::GrantGE(const FSynergyApplyParams& Params)
{
	if (!SynergyData || !SynergyData->IsValidData())
		return;
	if (!Params.Instigator || !Params.Instigator->HasAuthority())
		return;

	const int32 DefaultTier = SynergyData->ComputeActiveTierIndex(Params.Count);
	const int32 TierIdx = ComputeActiveTierIndex(Params, DefaultTier);

	// 시너지 티어가 바뀌었을 경우 기존에 부여한 GE 회수
	if (CachedTierIndex != TierIdx)
	{
		RevokeAllGrantGEs();
		CachedTierIndex = TierIdx;
	}
	
	const FSynergyTier* Tier = SynergyData->GetTier(TierIdx);
	if (!Tier)
		return;

	if (APCCombatGameState* GS = GetWorld() ? GetWorld()->GetGameState<APCCombatGameState>() : nullptr)
	{
		// 전투중이 아닐 경우 수행
		if (!GS->GetGameStateTag().MatchesTagExact(GameStateTags::Game_State_Combat_Active))
		{
			// 아군 중 랜덤으로 적용하는 시너지일 경우 전투 시작 타이밍에 부여
			if (IsRandomAmongPolicy(Tier))
				return;
		}
	}
	
	if (!SynergyData->GetApplyEffects().IsEmpty())
	{
		GrantEffects(Params, Tier, TierIdx + 1);
	}
}

void UPCSynergyBase::CombatActiveGrant(const FSynergyApplyParams& Params)
{
	if (!SynergyData || !SynergyData->IsValidData())
		return;
	if (!Params.Instigator || !Params.Instigator->HasAuthority())
		return;

	const int32 DefaultTier = SynergyData->ComputeActiveTierIndex(Params.Count);
	const int32 TierIdx = ComputeActiveTierIndex(Params, DefaultTier);
	const FSynergyTier* Tier = SynergyData->GetTier(TierIdx);

	if (!Tier)
		return;
	
	if (IsRandomAmongPolicy(Tier))
	{
		RevokeAllGrantGEs();
		GrantEffects(Params, Tier, TierIdx + 1);
	}

	RevokeAllGrantGAs();
	GrantAbility(Params, Tier, TierIdx + 1);
}

void UPCSynergyBase::CombatEndRevoke()
{
	const FSynergyTier* Tier = SynergyData->GetTier(CachedTierIndex);
	if (!Tier)
		return;

	// 라운드 종료 시 랜덤 유닛에게 부여한 GE일 경우 회수
	if (IsRandomAmongPolicy(Tier))
	{
		RevokeAllGrantGEs();
	}

	// 부여한 GA 회수
	RevokeAllGrantGAs();
}

void UPCSynergyBase::ResetAll()
{
	RevokeAllGrantGAs();
	RevokeAllGrantGEs();
	CachedTierIndex = -1;
}

void UPCSynergyBase::PlayActiveParticleAtUnit(const FSynergyApplyParams& Params) const
{
	if (!SynergyData || !SynergyData->IsValidData())
		return;
	if (!Params.Instigator || !Params.Instigator->HasAuthority())
		return;

	const int32 DefaultTier = SynergyData->ComputeActiveTierIndex(Params.Count);
	const int32 TierIdx = ComputeActiveTierIndex(Params, DefaultTier);

	// 시너지가 활성화 되지 않았다면 이펙트 재생 X
	if (TierIdx == -1)
		return;

	const TArray<APCHeroUnitCharacter*>& Units = Params.Units;
	for (APCHeroUnitCharacter* Hero : Units)
	{
		if (!Hero)
			continue;

		UAbilitySystemComponent* ASC = Hero->GetAbilitySystemComponent();
		if (!ASC)
			return;
		
		if (ASC->HasMatchingGameplayTag(SynergyData->GetSynergyTag()))
		{
			FGameplayCueParameters CueParams;
			CueParams.Location = Hero->GetActorLocation();
			CueParams.SourceObject = SynergyData->GetActiveParticle();
		
			ASC->ExecuteGameplayCue(GameplayCueTags::GameplayCue_VFX_Unit_SynergyActive, CueParams);
		}
	}
}

void UPCSynergyBase::SelectRecipients(const FSynergyApplyParams& Params, const FSynergyTier& Tier,
                                      TArray<APCHeroUnitCharacter*>& OutRecipients) const
{
	OutRecipients.Reset();

	switch (Tier.RecipientPolicy)
	{
		// 시너지 보유 유닛 전부
	case ESynergyRecipientPolicy::AllOwners:
		{
			for (APCHeroUnitCharacter* Hero : Params.Units)
			{
				const UAbilitySystemComponent* ASC = Hero ? Hero->GetAbilitySystemComponent() : nullptr;
				if (ASC && ASC->HasMatchingGameplayTag(SynergyData->GetSynergyTag()))
					OutRecipients.Add(Hero);
			}
			break;
		}
		// 시너지를 보유한 유닛 중 랜덤
	case ESynergyRecipientPolicy::RandomAmongOwners:
		{
			// 시너지를 보유한 유닛만 선별
			TArray<APCHeroUnitCharacter*> Owners;
			for (APCHeroUnitCharacter* Hero : Params.Units)
			{
				const UAbilitySystemComponent* ASC = Hero ? Hero->GetAbilitySystemComponent() : nullptr;
				if (ASC && ASC->HasMatchingGameplayTag(SynergyData->GetSynergyTag()))
					Owners.Add(Hero);
			}
			// 선별한 유닛들 랜덤으로 섞고 앞에서부터 뽑음
			for (int32 i=0; i<Owners.Num(); ++i)
			{
				const int32 SwapIdx = FMath::RandRange(i, Owners.Num()-1);
				if (i != SwapIdx) Owners.Swap(i, SwapIdx);
			}
			const int32 PickCnt = FMath::Clamp(Tier.RandomPickCount, 1, Owners.Num());
			for (int32 i=0; i<PickCnt; ++i)
				OutRecipients.Add(Owners[i]);
			break;
		}

	case ESynergyRecipientPolicy::RandomAmongAllies:
		{
			TArray<APCHeroUnitCharacter*> Allies = Params.Units;

			// 유닛들 랜덤으로 섞고 앞에서부터 뽑음
			for (int32 i=0; i<Allies.Num(); ++i)
			{
				const int32 SwapIdx = FMath::RandRange(i, Allies.Num()-1);
				if (i != SwapIdx) Allies.Swap(i, SwapIdx);
			}
			const int32 PickCnt = FMath::Clamp(Tier.RandomPickCount, 1, Allies.Num());
			for (int32 i=0; i<PickCnt; ++i)
				OutRecipients.Add(Allies[i]);
			break;
		}
		
		// 모든 아군
	case ESynergyRecipientPolicy::AllAllies:
	default:
		OutRecipients = Params.Units;
		break;
	}
}

void UPCSynergyBase::RevokeAllGrantGAs()
{
	for (auto& KV : ActiveGrantGAs)
	{
		if (UAbilitySystemComponent* ASC = KV.Key.Get())
		{
			for (const FGameplayAbilitySpecHandle& Handle : KV.Value)
			{
				if (Handle.IsValid())
					ASC->ClearAbility(Handle);
			}
		}
	}
	ActiveGrantGAs.Reset();
}

void UPCSynergyBase::RevokeAllGrantGEs()
{
	for (auto& KV : ActiveGrantGEs)
	{
		if (UAbilitySystemComponent* ASC = KV.Key.Get())
		{
			for (const FActiveGameplayEffectHandle& Handle : KV.Value)
			{
				if (Handle.IsValid())
					ASC->RemoveActiveGameplayEffect(Handle);
			}
		}
	}
	ActiveGrantGEs.Reset();
}

void UPCSynergyBase::RevokeHeroGrantGAs(const APCHeroUnitCharacter* Hero)
{
	if (!Hero || !Hero->HasAuthority())
		return;
	
	UAbilitySystemComponent* ASC = Hero->GetAbilitySystemComponent();
	if (!ASC)
		return;
	
	if (TArray<FGameplayAbilitySpecHandle>* ActiveGAs = ActiveGrantGAs.Find(ASC))
	{
		for (const auto& Handle : *ActiveGAs)
		{
			if (Handle.IsValid())
				ASC->ClearAbility(Handle);
		}
	
		ActiveGrantGAs.Remove(ASC);
	}
}

void UPCSynergyBase::RevokeHeroGrantGEs(const APCHeroUnitCharacter* Hero)
{
	if (!Hero || !Hero->HasAuthority())
		return;
	
	UAbilitySystemComponent* ASC = Hero->GetAbilitySystemComponent();
	if (!ASC)
		return;
	
	if (TArray<FActiveGameplayEffectHandle>* ActiveGEs = ActiveGrantGEs.Find(ASC))
	{
		for (const auto& Handle : *ActiveGEs)
		{
			if (Handle.IsValid())
				ASC->RemoveActiveGameplayEffect(Handle);
		}

		ActiveGrantGEs.Remove(ASC);
	}
}


void UPCSynergyBase::GrantAbility(const FSynergyApplyParams& Params, const FSynergyTier* Tier, int32 Level)
{
	if (!Tier)
		return;

	TArray<APCHeroUnitCharacter*> Recipients;
	SelectRecipients(Params, *Tier, Recipients);
	if (Recipients.Num() == 0)
		return;

	for (APCHeroUnitCharacter* Hero : Recipients)
	{
		if (!Hero) continue;
		if (UAbilitySystemComponent* HeroASC = Hero->GetAbilitySystemComponent())
		{
			// 이미 부여돼있을 경우 스킵 (라운드 종료 시 초기화 하기 때문에 부여돼있을 경우는 없지만 방어코드)
			if (ActiveGrantGAs.Contains(HeroASC))
				continue;
			
			TArray<FGameplayAbilitySpecHandle>& Handles = ActiveGrantGAs.FindOrAdd(HeroASC);

			for (const FSynergyGrantGA& GrantGA : SynergyData->GetGrantGAs())
			{
				if (!GrantGA.IsValid()) continue;
				
				UObject* SourceObject = SynergyData->GetUnitAbilityConfig();

				FGameplayAbilitySpec Spec(GrantGA.AbilityClass, Level, INDEX_NONE, SourceObject);

				const FGameplayAbilitySpecHandle Handle = HeroASC->GiveAbility(Spec);
				if (Handle.IsValid())
					Handles.Add(Handle);
			}
		}
	}
}

void UPCSynergyBase::GrantEffects(const FSynergyApplyParams& Params, const FSynergyTier* Tier, int32 Level)
{
	if (!Tier)
		return;

	TArray<APCHeroUnitCharacter*> Recipients;
	SelectRecipients(Params, *Tier, Recipients);
	if (Recipients.Num() == 0) return;

	for (const APCHeroUnitCharacter* Hero : Recipients)
	{
		if (!Hero) continue;

		if (UAbilitySystemComponent* HeroASC = Hero->GetAbilitySystemComponent())
		{
			// 이미 부여돼있을 경우 스킵
			if (ActiveGrantGEs.Contains(HeroASC))
				continue;
			
			TArray<FActiveGameplayEffectHandle>& Handles = ActiveGrantGEs.FindOrAdd(HeroASC);

			for (const auto& EffectSpec : SynergyData->GetApplyEffects().EffectSpecs)
			{
				if (!EffectSpec) continue;
				
				const FActiveGameplayEffectHandle Handle = EffectSpec->ApplyEffectSelf(HeroASC, Level);
				
				if (Handle.IsValid())
					Handles.Add(Handle);
			}
		}
	}
}

bool UPCSynergyBase::IsRandomAmongPolicy(const FSynergyTier* SynergyTier) const
{
	const ESynergyRecipientPolicy& RecipientPolicy = SynergyTier->RecipientPolicy;

	return RecipientPolicy == ESynergyRecipientPolicy::RandomAmongAllies || RecipientPolicy == ESynergyRecipientPolicy::RandomAmongOwners;
}
