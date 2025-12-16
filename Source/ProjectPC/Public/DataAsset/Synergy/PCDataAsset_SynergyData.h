// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Unit/EffectSpec/PCEffectCoreTypes.h"
#include "PCDataAsset_SynergyData.generated.h"

class UPCDataAsset_UnitAbilityConfig;

UENUM(BlueprintType)
enum class ESynergyRecipientPolicy : uint8
{
	// 시너지 보유 유닛 중 랜덤 N명
	RandomAmongOwners UMETA(DisplayName="Random Among Owners"),
	
	// 시너지 보유 유닛 전체
	AllOwners UMETA(DisplayName="All Owners"),

	// 모든 아군 유닛 중 랜덤 N명
	RandomAmongAllies UMETA(DisplayName="Random Among Allies"),
	
	// 모든 아군 (시너지 보유 여부 무관)
	AllAllies UMETA(DisplayName="All Allies"),
	
	// 커스텀 (코드에서 별도 정책 적용)
	Custom UMETA(DisplayName="Custom (Code)")
};

USTRUCT(BlueprintType)
struct FSynergyGrantGA
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, meta=(Categories="Unit.Ability"))
	FGameplayTag AbilityTag;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayAbility> AbilityClass = nullptr;
	
	bool IsValid() const
	{
		return AbilityTag.IsValid() && AbilityClass != nullptr;
	}
};

USTRUCT(BlueprintType)
struct FSynergyTier
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, meta=(ClampMin="1"))
	int32 Threshold = 1;

	UPROPERTY(EditDefaultsOnly)
	ESynergyRecipientPolicy RecipientPolicy = ESynergyRecipientPolicy::AllOwners;

	UPROPERTY(EditDefaultsOnly, meta=(EditCondition="(RecipientPolicy==ESynergyRecipientPolicy::RandomAmongOwners || RecipientPolicy==ESynergyRecipientPolicy::RandomAmongAllies)", ClampMin="1"))
	int32 RandomPickCount = 1;
};
/**
 * 
 */
UCLASS()
class PROJECTPC_API UPCDataAsset_SynergyData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditDefaultsOnly, meta=(Categories="Synergy"))
	FGameplayTag SynergyTag;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UPCDataAsset_UnitAbilityConfig> UnitAbilityConfig = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TArray<FSynergyTier> Tiers;

	UPROPERTY(EditDefaultsOnly, Category="Synergy|Payload")
	FPCEffectSpecList ApplyEffects;
	
	UPROPERTY(EditDefaultsOnly, Category="Synergy|Payload")
	TArray<FSynergyGrantGA> GrantGAs;
	
	UPROPERTY(EditDefaultsOnly, Category="Synergy|Effect")
	TObjectPtr<UParticleSystem> ActiveParticle = nullptr;
	
public:
	int32 ComputeActiveTierIndex(int32 CurrentCount) const;

	const FGameplayTag& GetSynergyTag() const { return SynergyTag; }
	UPCDataAsset_UnitAbilityConfig* GetUnitAbilityConfig() const { return UnitAbilityConfig; }

	const FPCEffectSpecList& GetApplyEffects() const { return ApplyEffects; }
	const TArray<FSynergyGrantGA>& GetGrantGAs() const { return GrantGAs; }
	
	const FSynergyTier* GetTier(int32 TierIndex) const;
	const TArray<FSynergyTier>& GetAllTiers() const { return Tiers; }

	UParticleSystem* GetActiveParticle() const { return ActiveParticle; }
	
	bool IsValidData() const
	{
		return SynergyTag.IsValid() && Tiers.Num() > 0;
	}
};
