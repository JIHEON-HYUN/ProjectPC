// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Unit/PCHeroUnitCharacter.h"

#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"

#include "AbilitySystem/Unit/PCHeroUnitAbilitySystemComponent.h"
#include "AbilitySystem/Unit/AttributeSet/PCHeroUnitAttributeSet.h"
#include "BaseGameplayTags.h"
#include "Component/PCSynergyComponent.h"
#include "Component/PCUnitEquipmentComponent.h"
#include "Controller/Unit/PCUnitAIController.h"
#include "DataAsset/Unit/PCDataAsset_HeroUnitData.h"
#include "GameFramework/WorldSubsystem/PCUnitSpawnSubsystem.h"
#include "UI/Unit/PCHeroStatusBarWidget.h"
#include "UI/Unit/PCUnitStatusBarWidget.h"
#include "Sound/SoundBase.h"


APCHeroUnitCharacter::APCHeroUnitCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
	HeroUnitAbilitySystemComponent = CreateDefaultSubobject<UPCHeroUnitAbilitySystemComponent>(TEXT("HeroUnitAbilitySystemComponent"));

	if (HeroUnitAbilitySystemComponent)
	{
		HeroUnitAbilitySystemComponent->SetIsReplicated(true);
		HeroUnitAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

		UPCHeroUnitAttributeSet* HeroAttrSet = CreateDefaultSubobject<UPCHeroUnitAttributeSet>(TEXT("HeroUnitAttributeSet"));
		HeroUnitAbilitySystemComponent->AddAttributeSetSubobject(HeroAttrSet);
		HeroUnitAttributeSet = HeroUnitAbilitySystemComponent->GetSet<UPCHeroUnitAttributeSet>();
	}
}

void APCHeroUnitCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APCHeroUnitCharacter, HeroLevel);
	DOREPLIFETIME(APCHeroUnitCharacter, bIsDragging);
}

UPCHeroUnitAbilitySystemComponent* APCHeroUnitCharacter::GetHeroUnitAbilitySystemComponent() const
{
	return HeroUnitAbilitySystemComponent;
}

const UPCHeroUnitAttributeSet* APCHeroUnitCharacter::GetHeroUnitAttributeSet()
{
	if (!HeroUnitAttributeSet)
	{
		if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		{
			HeroUnitAttributeSet = ASC->GetSet<UPCHeroUnitAttributeSet>();
		}
	}
	return HeroUnitAttributeSet;
}

UPCUnitAbilitySystemComponent* APCHeroUnitCharacter::GetUnitAbilitySystemComponent() const
{
	return HeroUnitAbilitySystemComponent;
}

void APCHeroUnitCharacter::LevelUp()
{
	// LevelUp은 서버권한
	if (!HasAuthority() || !HeroUnitAbilitySystemComponent)
		return;
	
	HeroLevel = FMath::Clamp(++HeroLevel, 1, 3);
	HeroUnitAbilitySystemComponent->UpdateGAS();
	
	// Listen Server인 경우 OnRep 수동 호출 (Listen Server 환경 대응, OnRep_HeroLevel 이벤트 못받기 때문)
	if (GetNetMode() == NM_ListenServer)
		OnRep_HeroLevel();

	// 한 프레임 내에서 뒤이어 Combine()이 호출 될 수도 있으니 레벨업 이펙트 생성을 다음 프레임으로 보류
	GetWorldTimerManager().SetTimerForNextTick([this]()
	{
		if (!bDidCombine)
		{
			PlayLevelUpParticle();
		}
	});
}

void APCHeroUnitCharacter::UpdateStatusBarUI() const
{
	if (UPCHeroStatusBarWidget* StatusBar = Cast<UPCHeroStatusBarWidget>(StatusBarComp->GetUserWidgetObject()))
	{
		StatusBar->SetVariantByLevel(HeroLevel);
	}
}

void APCHeroUnitCharacter::UpdateMeshScale() const
{
	if (HasAuthority() || !GetMesh())
		return;
	
	FVector MeshScale = FVector::OneVector;
	float IncreaseSize = FMath::Max(0.f,0.12 * (GetUnitLevel() - 1));
	FVector IncreaseSizeVector = FVector(IncreaseSize, IncreaseSize, IncreaseSize);
	MeshScale += IncreaseSizeVector;

	GetMesh()->SetRelativeScale3D(MeshScale);
}

void APCHeroUnitCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->HideBoneByName(TEXT("sheild_main"), PBO_None); // Steel 방패 제거 
		MeshComp->HideBoneByName(TEXT("arm_chain_long_r_01"), PBO_None); // Riktor 왼쪽 체인 비정상적인 메쉬 제거
		MeshComp->HideBoneByName(TEXT("wing_l_01"), PBO_None); // Serath 날개 제거
		MeshComp->HideBoneByName(TEXT("wing_r_01"), PBO_None);
		MeshComp->HideBoneByName(TEXT("ghostbeast_root"), PBO_None); // Khaimera Ghost Beast 소켓 제거 
	}
	
	if (HasAuthority())
	{
		if (auto* ASC = GetAbilitySystemComponent())
		{
			SynergyTagChangedHandle = ASC->RegisterGameplayTagEvent(SynergyGameplayTags::Synergy, EGameplayTagEventType::AnyCountChange)
			.AddUObject(this, &ThisClass::OnSynergyTagChanged);
		}

		if (!bDidPlaySpawnSound)
		{
			UPCUnitSpawnSubsystem* SpawnSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UPCUnitSpawnSubsystem>() : nullptr;
			if (SpawnSubsystem && OwnerPS && OwnerPS->GetAbilitySystemComponent())
			{
				bDidPlaySpawnSound = true;
				
				if (USoundBase* LevelStartSound = SpawnSubsystem->GetLevelStartSoundCueByUnitTag(UnitTag))
				{
					FGameplayCueParameters Params;
					Params.SourceObject = LevelStartSound;
		
					OwnerPS->GetAbilitySystemComponent()->ExecuteGameplayCue(GameplayCueTags::GameplayCue_SFX_Unit_LevelStart, Params);
				}
			}
		}
	}
}

void APCHeroUnitCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnHeroDestroyed.Broadcast(this);
	
	if (SynergyTagChangedHandle.IsValid())
		GetAbilitySystemComponent()->RegisterGameplayTagEvent(SynergyGameplayTags::Synergy).Remove(SynergyTagChangedHandle);
	
	Super::EndPlay(EndPlayReason);
	
}

void APCHeroUnitCharacter::RestoreFromCombatEnd()
{
	if (HasAuthority() && bIsOnField)
	{
		bIsCombatWin = false;
		
		if (!HeroUnitAbilitySystemComponent || !HeroUnitAttributeSet)
			return;
		
		// Max Health 값으로 Current Health 초기화
		HeroUnitAbilitySystemComponent->ApplyModToAttribute(
			UPCUnitAttributeSet::GetCurrentHealthAttribute(),
			EGameplayModOp::Override,
			HeroUnitAttributeSet->GetMaxHealth());

		if (!HeroUnitDataAsset)
			return;

		// 기본으로 가지는 Current Mana로 초기화
		HeroUnitAbilitySystemComponent->ApplyModToAttribute(
			UPCHeroUnitAttributeSet::GetCurrentManaAttribute(),
			EGameplayModOp::Override,
			HeroUnitAttributeSet->GetCombatStartMana());

		// 이전 전투에서 사망했을 경우 사망 태그 제거
		if (HeroUnitAbilitySystemComponent->HasMatchingGameplayTag(UnitGameplayTags::Unit_State_Combat_Dead))
		{
			HeroUnitAbilitySystemComponent->RemoveLooseGameplayTag(UnitGameplayTags::Unit_State_Combat_Dead);
		}

		// 스턴 상태일 경우 스턴 태그 제거
		if (HeroUnitAbilitySystemComponent->HasMatchingGameplayTag(UnitGameplayTags::Unit_State_Combat_Stun))
		{
			HeroUnitAbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(
				FGameplayTagContainer(UnitGameplayTags::Unit_State_Combat_Stun));
		}
		
		// 블랙보드 키값 초기화
		if (APCUnitAIController* AIC = Cast<APCUnitAIController>(GetController()))
		{
			AIC->ClearBlackboardValue();
		}
	}
}

void APCHeroUnitCharacter::ChangedOnTile(const bool IsOnField)
{
	if (UPCSynergyComponent* SynergyComp = OwnerPS ? OwnerPS->GetSynergyComponent() : nullptr)
	{
		if (bIsOnField && !IsOnField)
		{
			SynergyComp->UnRegisterHero(this);
		}
		else if (!bIsOnField && IsOnField)
		{
			SynergyComp->RegisterHero(this);
		}
	}
	
	Super::ChangedOnTile(IsOnField);
}

void APCHeroUnitCharacter::ActionDrag(const bool IsStart)
{
	// 서버에서만 실행
	if (!HasAuthority())
		return;

	bIsDragging = IsStart;
}

void APCHeroUnitCharacter::OnRep_IsDragging() const
{
	SetMeshVisibility(!bIsDragging);
}

void APCHeroUnitCharacter::OnSynergyTagChanged(const FGameplayTag Tag, int32 NewCount) const
{
	// 벤치에 있는 경우 해당 이벤트 무시
	if (!bIsOnField)
		return;
	
	if (Tag.MatchesTag(SynergyGameplayTags::Synergy))
	{
		OnHeroSynergyTagChanged.Broadcast(this);
	}
}

void APCHeroUnitCharacter::OnRep_HeroLevel()
{
	// 클라에서 플레이어에게 보여주는 로직 ex) Status Bar UI 체인지
	UpdateStatusBarUI();
	UpdateMeshScale();
	OnHeroLevelUp.Broadcast();
}

void APCHeroUnitCharacter::PlayLevelUpParticle() const
{
	FGameplayCueParameters Params;
	Params.TargetAttachComponent = GetMesh();
	HeroUnitAbilitySystemComponent->ExecuteGameplayCue(GameplayCueTags::GameplayCue_VFX_Unit_LevelUp, Params);
}

void APCHeroUnitCharacter::OnGameStateChanged(const FGameplayTag& NewStateTag)
{
	Super::OnGameStateChanged(NewStateTag);
	
	const FGameplayTag& CombatActiveTag = GameStateTags::Game_State_Combat_Active;
	const FGameplayTag& CombatEndTag = GameStateTags::Game_State_Combat_End;

	if (NewStateTag == CombatActiveTag)
	{
		//SetMeshVisibility(true);
	}
	else if (NewStateTag.MatchesTag(CombatEndTag))
	{
		if (bIsOnField)
		{
			RestoreFromCombatEnd();
			SetMeshVisibility(true);
		}
	}
}

void APCHeroUnitCharacter::SetUnitLevel(const int32 Level)
{
	// 레벨 데이터 수정은 서버권한
	if (!HasAuthority() || !HeroUnitAbilitySystemComponent)
		return;
	
	HeroLevel = FMath::Clamp(Level, 1, 3);
	HeroUnitAbilitySystemComponent->UpdateGAS();
	
	// Listen Server인 경우 OnRep 수동 호출 (Listen Server 환경 대응, OnRep_HeroLevel 이벤트 못받기 때문)
	if (GetNetMode() == NM_ListenServer)
		OnRep_HeroLevel();
}

void APCHeroUnitCharacter::Combine(const APCHeroUnitCharacter* LevelUpHero)
{
	if (LevelUpHero)
	{
		if (UPCUnitEquipmentComponent* TargetEquipmentComp = LevelUpHero->EquipmentComp)
		{
			TargetEquipmentComp->UnionEquipmentComponent(EquipmentComp);
		}
	}

	if (UAbilitySystemComponent* OwnerASC = OwnerPS ? OwnerPS->GetAbilitySystemComponent() : nullptr)
	{
		FGameplayCueParameters Params;
		Params.Location = GetActorLocation() + FVector(0.f,0.f,80.f);
		OwnerASC->ExecuteGameplayCue(GameplayCueTags::GameplayCue_VFX_Unit_Combine, Params);
	}
	
	bDidCombine = true;
	
	Destroy();
}

void APCHeroUnitCharacter::SellHero()
{
	if (EquipmentComp)
	{
		EquipmentComp->ReturnAllItemToPlayerInventory(true);
	}

	Destroy();
}

void APCHeroUnitCharacter::SetUnitDataAsset(UPCDataAsset_BaseUnitData* InUnitDataAsset)
{
	HeroUnitDataAsset = Cast<UPCDataAsset_HeroUnitData>(InUnitDataAsset);

	if (HeroUnitDataAsset)
	{
		SetUnitRecommendedPosition(HeroUnitDataAsset->GetRecommentPosition());
	}
}

void APCHeroUnitCharacter::InitStatusBarWidget(UUserWidget* StatusBarWidget)
{
	// 데디서버거나 StatusBar Class가 없으면 실행하지 않음, HasAuthority() 안쓰는 이유: Listen Server 환경 고려
	if (GetNetMode() == NM_DedicatedServer || !StatusBarClass)
		return;

	if (UPCHeroStatusBarWidget* StatusBar = Cast<UPCHeroStatusBarWidget>(StatusBarWidget))
	{
		StatusBar->InitWithASC(this,GetAbilitySystemComponent(),
			UPCHeroUnitAttributeSet::GetCurrentHealthAttribute(),
			UPCHeroUnitAttributeSet::GetMaxHealthAttribute(),
			UPCHeroUnitAttributeSet::GetCurrentManaAttribute(),
			UPCHeroUnitAttributeSet::GetMaxManaAttribute(),
			HeroLevel);
	}
	
}
