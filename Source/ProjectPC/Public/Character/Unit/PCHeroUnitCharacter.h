// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Unit/PCBaseUnitCharacter.h"
#include "DataAsset/Unit/PCDataAsset_HeroUnitData.h"
#include "PCHeroUnitCharacter.generated.h"

class UPCHeroUnitAttributeSet;
class UPCHeroUnitAbilitySystemComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHeroDestroyed, APCHeroUnitCharacter*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHeroSynergyTagChanged, const APCHeroUnitCharacter*);
DECLARE_MULTICAST_DELEGATE(FOnHeroLevelUp);

/**
 * 
 */
UCLASS()
class PROJECTPC_API APCHeroUnitCharacter : public APCBaseUnitCharacter
{
	GENERATED_BODY()

public:
	APCHeroUnitCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UPCHeroUnitAbilitySystemComponent* GetHeroUnitAbilitySystemComponent() const;
	const UPCHeroUnitAttributeSet* GetHeroUnitAttributeSet();
	virtual UPCUnitAbilitySystemComponent* GetUnitAbilitySystemComponent() const override;
	
	virtual bool HasLevelSystem() const override { return true; }
	virtual int32 GetUnitLevel() const override { return HeroLevel; };
	void Combine(const APCHeroUnitCharacter* LevelUpHero);
	void SellHero();
	void LevelUp();
	
	virtual UPCDataAsset_BaseUnitData* GetUnitDataAsset() const override { return HeroUnitDataAsset; }
	virtual void SetUnitDataAsset(UPCDataAsset_BaseUnitData* InUnitDataAsset) override;
	
protected:
	virtual void SetUnitLevel(const int32 Level) override;
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void InitStatusBarWidget(UUserWidget* StatusBarWidget) override;
	void UpdateStatusBarUI() const;
	void UpdateMeshScale() const;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
	TObjectPtr<UPCHeroUnitAbilitySystemComponent> HeroUnitAbilitySystemComponent;

	UPROPERTY(Transient)
	TObjectPtr<const UPCHeroUnitAttributeSet> HeroUnitAttributeSet = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Data")
	TObjectPtr<UPCDataAsset_HeroUnitData> HeroUnitDataAsset;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_HeroLevel, meta=(ExposeOnSpawn=true), Category="Data")
	int32 HeroLevel = 1;

	UFUNCTION()
	virtual void OnRep_HeroLevel();

	bool bDidCombine = false;
	bool bDidPlaySpawnSound = false;
	
private:
	void PlayLevelUpParticle() const;
	
	UPROPERTY(ReplicatedUsing=OnRep_IsDragging)
	bool bIsDragging;

	UFUNCTION()
	void OnRep_IsDragging() const;
	
	// 전투 관련 //
protected:
	virtual void OnGameStateChanged(const FGameplayTag& NewStateTag) override;

private:
	void RestoreFromCombatEnd();
	
public:
	virtual void ChangedOnTile(const bool IsOnField) override;
	
	UFUNCTION(BlueprintCallable, Category="DragAndDrop")
	void ActionDrag(const bool IsStart);
	
	// 시너지, UI 관련 //
public:
	FOnHeroDestroyed OnHeroDestroyed;
	FOnHeroSynergyTagChanged OnHeroSynergyTagChanged;
	FDelegateHandle SynergyTagChangedHandle;
	FOnHeroLevelUp OnHeroLevelUp;
	
private:
	void OnSynergyTagChanged(const FGameplayTag Tag, int32 NewCount) const;
};