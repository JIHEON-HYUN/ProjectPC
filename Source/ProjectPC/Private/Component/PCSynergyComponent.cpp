// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PCSynergyComponent.h"

#include "AbilitySystemComponent.h"
#include "BaseGameplayTags.h"
#include "Character/Unit/PCHeroUnitCharacter.h"
#include "DataAsset/Synergy/PCDataAsset_SynergyData.h"
#include "DataAsset/Synergy/PCDataAsset_SynergyDefinitionSet.h"
#include "GameFramework/GameState/PCCombatGameState.h"
#include "Net/UnrealNetwork.h"
#include "Synergy/PCSynergyBase.h"

void FHeroSynergyTally::IncreaseSynergyTag(const FGameplayTag& SynergyTag, bool& OutIsUnique)
{
	int32& Count = SynergyCountMap.FindOrAdd(SynergyTag);
	Count++;

	if (Count == 1)
		OutIsUnique = true;
}

void FHeroSynergyTally::IncreaseSynergyTags(const FGameplayTagContainer& SynergyTags,
	FGameplayTagContainer& OutNewSynergyTags)
{
	for (const FGameplayTag& SynergyTag : SynergyTags)
	{
		int32& Count = SynergyCountMap.FindOrAdd(SynergyTag);
		Count++;

		if (Count == 1)
			OutNewSynergyTags.AddTag(SynergyTag);
	}
}

void FHeroSynergyTally::DecreaseSynergyTag(const FGameplayTag& SynergyTag, bool& OutIsRemoved)
{
	int32& Count = SynergyCountMap.FindOrAdd(SynergyTag);
	Count--;

	if (Count <= 0)
	{
		SynergyCountMap.Remove(SynergyTag);
		OutIsRemoved = true;
	}
}

FGameplayTagContainer FHeroSynergyTally::GetActiveSynergyTags()
{
	FGameplayTagContainer ActiveSynergyTags;
	for (const auto& Pair : SynergyCountMap)
	{
		if (Pair.Value > 0)
			ActiveSynergyTags.AddTag(Pair.Key);
	}
	
	return ActiveSynergyTags;
}

UPCSynergyComponent::UPCSynergyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SynergyCountMap.Reset();
	SetIsReplicatedByDefault(true);
}

void UPCSynergyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPCSynergyComponent, SynergyCountArray);
}

void UPCSynergyComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeSynergyHandlersFromDefinitionSet();
	
	if (GetOwner()->HasAuthority())
	{
		BindGameStateDelegates();
	}
}

void UPCSynergyComponent::InitializeSynergyHandlersFromDefinitionSet()
{
	SynergyToTagMap.Empty();

	if (!SynergyDefinitionSet)
		return;

	for (const FSynergyDefinition& Def : SynergyDefinitionSet->Definitions)
	{
		if (!Def.SynergyClass || !Def.SynergyData)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Synergy] Invalid Definition (class/data missing)"));
			continue;
		}

		const FGameplayTag Key = Def.SynergyData->GetSynergyTag();
		if (!Key.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Synergy] Definition has InValid SynergyTag"));
			continue;
		}
		if (SynergyToTagMap.Contains(Key))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Synergy] Duplicate SynergyTag: %s. Skipping"), *Key.ToString());
			continue;
		}

		UPCSynergyBase* SynergyBase = NewObject<UPCSynergyBase>(this, Def.SynergyClass);
		SynergyBase->SetSynergyData(Def.SynergyData);

		SynergyToTagMap.Add(Key, SynergyBase);
	}
}


void UPCSynergyComponent::OnRep_SynergyCountArray()
{
	SynergyData.Reset(SynergyCountArray.Entries.Num());

	for (const FSynergyCountEntry& Entry : SynergyCountArray.Entries)
	{
		if (!Entry.Tag.IsValid())
			continue;

		FSynergyData Data;
		Data.SynergyTag = Entry.Tag;
		Data.Count = Entry.Count;
		Data.Thresholds = GetSynergyThresholds(Entry.Tag);
		Data.TierIndex = GetSynergyTierIndexFromCount(Entry.Tag, Data.Count);

		SynergyData.Add(MoveTemp(Data));
	}

	OnSynergyCountsChanged.Broadcast(SynergyData);
}

void UPCSynergyComponent::RegisterHero(APCHeroUnitCharacter* Hero)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Hero)
		return;

	if (!RegisterHeroSet.Contains(Hero))
	{
		RegisterHeroSet.Add(Hero);
		Hero->OnHeroDestroyed.AddUObject(this, &ThisClass::OnHeroDestroyed);
		Hero->OnHeroSynergyTagChanged.AddUObject(this, &ThisClass::OnHeroSynergyTagChanged);
		
		FGameplayTagContainer SynergyTags;
		GetHeroSynergyTags(Hero, SynergyTags);

		const FGameplayTag HeroTag = Hero->GetUnitTag();
		FHeroSynergyTally& SynergyTally = HeroSynergyMap.FindOrAdd(HeroTag);
		FGameplayTagContainer NewSynergyTags;

		SynergyTally.IncreaseSynergyTags(SynergyTags, NewSynergyTags);
		
		if (!NewSynergyTags.IsEmpty())
		{
			UpdateSynergyCountMap(NewSynergyTags, true);
		}

		for (const FGameplayTag& SynergyTag : SynergyTags)
		{
			ApplySynergyEffects(SynergyTag);
			
			GetWorld()->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateUObject(this, &UPCSynergyComponent::PlaySynergyActiveParticle, SynergyTag)
				);
		}
	}
}

void UPCSynergyComponent::UnRegisterHero(APCHeroUnitCharacter* Hero)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Hero)
		return;

	if (RegisterHeroSet.Contains(Hero))
	{
		RegisterHeroSet.Remove(Hero);
		Hero->OnHeroDestroyed.RemoveAll(this);
		Hero->OnHeroSynergyTagChanged.RemoveAll(this);
		
		FGameplayTagContainer SynergyTags;
		GetHeroSynergyTags(Hero, SynergyTags);

		const FGameplayTag HeroTag = Hero->GetUnitTag();
		FHeroSynergyTally& SynergyTally = HeroSynergyMap.FindOrAdd(HeroTag);
		FGameplayTagContainer RemoveSynergyTags;

		for (const FGameplayTag& SynergyTag : SynergyTags)
		{
			if (UPCSynergyBase* Synergy = *SynergyToTagMap.Find(SynergyTag))
			{
				Synergy->RevokeHeroGrantGEs(Hero);
				Synergy->RevokeHeroGrantGAs(Hero);
			}
			
			bool bIsRemoved = false;
			SynergyTally.DecreaseSynergyTag(SynergyTag, bIsRemoved);

			if (bIsRemoved)
				RemoveSynergyTags.AddTag(SynergyTag);
		}
		
		if (SynergyTally.IsEmpty())
			HeroSynergyMap.Remove(HeroTag);

		if (!RemoveSynergyTags.IsEmpty())
		{
			UpdateSynergyCountMap(RemoveSynergyTags, false);
		}

		for (const FGameplayTag& SynergyTag : SynergyTags)
		{
			ApplySynergyEffects(SynergyTag);
		}
	}
}

TArray<int32> UPCSynergyComponent::GetSynergyThresholds(const FGameplayTag& SynergyTag) const
{
	TArray<int32> Result;

	if (const UPCSynergyBase* Synergy = SynergyToTagMap.FindRef(SynergyTag))
	{
		const UPCDataAsset_SynergyData* InSynergyData = Synergy->GetSynergyData();
		for (const FSynergyTier& SynergyTier : InSynergyData->GetAllTiers())
		{
			Result.Add(SynergyTier.Threshold);
		}
	}

	return Result;
}

int32 UPCSynergyComponent::GetSynergyTierIndexFromCount(const FGameplayTag& SynergyTag, int32 Count) const
{
	int32 Result = -1;
	
	if (const UPCSynergyBase* Synergy = SynergyToTagMap.FindRef(SynergyTag))
	{
		if (const UPCDataAsset_SynergyData* InSynergyData = Synergy->GetSynergyData())
		{
			Result = InSynergyData->ComputeActiveTierIndex(Count);
		}
	}

	return Result;
}

void UPCSynergyComponent::UpdateSynergyCountMap(const FGameplayTagContainer& SynergyTags, const bool bRegisterHero)
{
	for (const FGameplayTag& SynergyTag : SynergyTags)
	{
		int32& SynergyCnt = SynergyCountMap.FindOrAdd(SynergyTag);
		if (bRegisterHero)
		{
			SynergyCnt++;
		}
		else
		{
			SynergyCnt--;
			if (SynergyCnt <= 0)
			{
				SynergyCountMap.Remove(SynergyTag);
			}
		}
	}

	SynergyCountArray.UpdateToMap(SynergyCountMap);
}

void UPCSynergyComponent::RecountSynergyCountMapForUnitTag(const FGameplayTag& UnitTag)
{
	TArray<APCHeroUnitCharacter*> Heroes;
	GatherRegisteredHeroes(Heroes);

	FHeroSynergyTally& SynergyTally = HeroSynergyMap.FindOrAdd(UnitTag);
	if (!SynergyTally.IsEmpty())
	{
		const FGameplayTagContainer ActiveSynergyTags = SynergyTally.GetActiveSynergyTags();
		for (const FGameplayTag& SynergyTag : ActiveSynergyTags)
		{
			int32& SynergyCnt = SynergyCountMap.FindOrAdd(SynergyTag);
			SynergyCnt--;
			if (SynergyCnt <= 0)
			{
				SynergyCountMap.Remove(SynergyTag);
			}
		}
		SynergyTally.Reset();
	}
	
	FGameplayTagContainer NewSynergyTags;
	
	for (const auto& Hero : Heroes)
	{
		if (Hero && Hero->GetUnitTag().MatchesTagExact(UnitTag))
		{
			FGameplayTagContainer SynergyTags;
			GetHeroSynergyTags(Hero, SynergyTags);

			SynergyTally.IncreaseSynergyTags(SynergyTags, NewSynergyTags);
		}
	}
	
	for (const FGameplayTag& SynergyTag : NewSynergyTags)
	{
		int32& SynergyCnt = SynergyCountMap.FindOrAdd(SynergyTag);
		SynergyCnt++;

		ApplySynergyEffects(SynergyTag);
		
		GetWorld()->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UPCSynergyComponent::PlaySynergyActiveParticle, SynergyTag)
			);
	}

	SynergyCountArray.UpdateToMap(SynergyCountMap);
}

void UPCSynergyComponent::ApplySynergyEffects(const FGameplayTag& SynergyTag)
{
	UPCSynergyBase* Synergy = *SynergyToTagMap.Find(SynergyTag);
	
	if (!Synergy)
		return;
	
	TArray<APCHeroUnitCharacter*> CurrentHeroes;
	GatherRegisteredHeroes(CurrentHeroes);

	AActor* Instigator = GetOwner();
	int32 Count = SynergyCountMap.FindRef(SynergyTag);
	
	FSynergyApplyParams Params;
	Params.SynergyTag = SynergyTag;
	Params.Count = Count;
	Params.Units = CurrentHeroes;
	Params.Instigator = Instigator;
		
	Synergy->GrantGE(Params);
}

void UPCSynergyComponent::PlaySynergyActiveParticle(FGameplayTag SynergyTag)
{
	UPCSynergyBase* Synergy = *SynergyToTagMap.Find(SynergyTag);
	
	if (!Synergy)
		return;
	
	TArray<APCHeroUnitCharacter*> CurrentHeroes;
	GatherRegisteredHeroes(CurrentHeroes);

	AActor* Instigator = GetOwner();
	int32 Count = SynergyCountMap.FindRef(SynergyTag);
	
	FSynergyApplyParams Params;
	Params.SynergyTag = SynergyTag;
	Params.Count = Count;
	Params.Units = CurrentHeroes;
	Params.Instigator = Instigator;
		
	Synergy->PlayActiveParticleAtUnit(Params);
}

void UPCSynergyComponent::BindGameStateDelegates()
{
	if (GetOwner()->HasAuthority())
	{
		if (APCCombatGameState* GS = GetWorld() ? GetWorld()->GetGameState<APCCombatGameState>() : nullptr)
		{
			GS->OnGameStateTagChanged.AddUObject(this, &ThisClass::OnGameStateTagChanged);
		}
	}
}

void UPCSynergyComponent::OnGameStateTagChanged(const FGameplayTag& NewTag)
{
	if (GetOwner()->HasAuthority())
	{
		if (NewTag.MatchesTag(GameStateTags::Game_State_Combat_Active))
		{
			OnCombatActiveAction();
		}
		else
		{
			OnCombatEndAction();
		}
	}
}

void UPCSynergyComponent::OnCombatActiveAction()
{
	TArray<APCHeroUnitCharacter*> CurrentHeroes;
	GatherRegisteredHeroes(CurrentHeroes);

	AActor* Instigator = GetOwner();

	for (auto& KV : SynergyToTagMap)
	{
		const FGameplayTag SynergyTag = KV.Key;
		UPCSynergyBase* Handler = KV.Value;
		if (!Handler)
			continue;

		const int32 Count = SynergyCountMap.FindRef(SynergyTag);

		FSynergyApplyParams Params;
		Params.SynergyTag = SynergyTag;
		Params.Count = Count;
		Params.Units = CurrentHeroes;
		Params.Instigator = Instigator;
		
		Handler->CombatActiveGrant(Params);
	}
}

void UPCSynergyComponent::OnCombatEndAction()
{
	for (auto& KV : SynergyToTagMap)
	{
		UPCSynergyBase* Handler = KV.Value;
		if (!Handler)
			continue;
		
		Handler->CombatEndRevoke();
	}
}

void UPCSynergyComponent::OnHeroDestroyed(APCHeroUnitCharacter* DestroyedHero)
{
	if (RegisterHeroSet.Contains(DestroyedHero))
	{
		UnRegisterHero(DestroyedHero);
	}
}

void UPCSynergyComponent::OnHeroSynergyTagChanged(const APCHeroUnitCharacter* Hero)
{
	if (Hero)
	{
		const FGameplayTag HeroTag = Hero->GetUnitTag();
		RecountSynergyCountMapForUnitTag(HeroTag);
	}
}

void UPCSynergyComponent::GetHeroSynergyTags(const APCHeroUnitCharacter* Hero, FGameplayTagContainer& OutSynergyTags) const
{
	OutSynergyTags.Reset();
	
	if (UAbilitySystemComponent* ASC = Hero ? Hero->GetAbilitySystemComponent() : nullptr)
	{
		FGameplayTagContainer Owned;
		ASC->GetOwnedGameplayTags(Owned);
		for (const FGameplayTag& Tag : Owned)
		{
			if (Tag.MatchesTag(SynergyGameplayTags::Synergy))
			{
				OutSynergyTags.AddTag(Tag);
			}
		}
	}
}

void UPCSynergyComponent::GatherRegisteredHeroes(TArray<APCHeroUnitCharacter*>& OutHeroes)
{
	OutHeroes.Reset();
	
	for (auto It = RegisterHeroSet.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
		else
		{
			OutHeroes.Add(It->Get());
		}
	}
}