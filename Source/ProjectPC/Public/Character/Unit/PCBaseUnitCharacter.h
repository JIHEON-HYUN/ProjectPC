// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "PCCommonUnitCharacter.h"
#include "DataAsset/Unit/PCDataAsset_BaseUnitData.h"
#include "GameFramework/HelpActor/PCCombatBoard.h"
#include "GameFramework/PlayerState/PCPlayerState.h"
#include "PCBaseUnitCharacter.generated.h"

class UPCUnitEquipmentComponent;
class UPCDataAsset_UnitAbilityConfig;
class UPCUnitStatusBarWidget;
class UWidgetComponent;
class UGameplayAbility;
class UPCUnitAttributeSet;
class UPCUnitAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitDied, APCBaseUnitCharacter*, Unit);

UCLASS()
class PROJECTPC_API APCBaseUnitCharacter : public APCCommonUnitCharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()
	
public:
	APCBaseUnitCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UPCUnitAbilitySystemComponent* GetUnitAbilitySystemComponent() const;
	UPCDataAsset_UnitAnimSet* GetUnitAnimSetDataAsset() const;
	const UPCDataAsset_UnitAbilityConfig* GetUnitAbilityConfigDataAsset() const;
	UPCDataAsset_ProjectileData* GetUnitProjectileDataAsset() const;
	
	virtual UPCDataAsset_BaseUnitData* GetUnitDataAsset() const;
	virtual void SetUnitDataAsset(UPCDataAsset_BaseUnitData* InUnitDataAsset) { }

	void SetStatusBarClass(const TSubclassOf<UUserWidget>& InStatusBarClass) { StatusBarClass = InStatusBarClass; }
	void SetOutlineMID(UMaterialInterface* OutlineMat);

	UFUNCTION(BlueprintCallable)
	void SetOutlineEnabled(bool bEnable) const;
	
	void SetOwnerPlayerState(APCPlayerState* InOwnerPS) { OwnerPS = InOwnerPS; }
	UFUNCTION(BlueprintPure)
	APCPlayerState* GetOwnerPlayerState() const { return OwnerPS; }

	virtual TArray<FGameplayTag> GetEquipItemTags() const override;
	
	FORCEINLINE UPCUnitEquipmentComponent* GetEquipmentComponent() const { return EquipmentComp; }
	FORCEINLINE const UWidgetComponent* GetStatusBarComponent() const { return StatusBarComp; }
	FORCEINLINE FName GetStatusBarSocketName() const { return StatusBarSocketName; }
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void InitStatusBarWidget(UUserWidget* StatusBarWidget) { }

	void ReAttachStatusBarToSocket() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UPCUnitEquipmentComponent> EquipmentComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> StatusBarComp;
	
	UPROPERTY(EditAnywhere, Category="Data")
	TSubclassOf<UUserWidget> StatusBarClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Data")
	FName StatusBarSocketName = TEXT("HealthBar");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Material Instance")
	TObjectPtr<UMaterialInstanceDynamic> OutlineMID = nullptr;
	
protected:

	UPROPERTY(EditDefaultsOnly, Replicated, Category="Data")
	int32 TeamIndex = -1;

	UPROPERTY(Transient)
	TObjectPtr<APCPlayerState> OwnerPS;
	
	virtual void OnRep_UnitTag() override;

	void PushTeamIndexToController() const;
	
	void InitAbilitySystem();
	void SetAnimSetData() const;

	void SetMeshVisibility(bool bVisibility) const;
	
	// 전투 시스템 관련 //
public:
	// Team Index, 위치한 CombatBoard 설정은 서버에서만 실행
	UFUNCTION(BlueprintCallable, Category="Combat")
	void SetTeamIndex(const int32 InTeamID) { if (HasAuthority()) TeamIndex = InTeamID; }

	UFUNCTION(BlueprintCallable, Category="Combat")
	int32 GetTeamIndex() const { return TeamIndex; }

	virtual FGenericTeamId GetGenericTeamId() const override;
	
	UFUNCTION(BlueprintCallable, Category="Combat")
	void SetOnCombatBoard(APCCombatBoard* InCombatBoard) { if (HasAuthority()) OnCombatBoard = InCombatBoard; }

	UFUNCTION(BlueprintCallable, Category="Combat")
	FORCEINLINE APCCombatBoard* GetOnCombatBoard() const { return OnCombatBoard.Get(); }

	UFUNCTION(BlueprintCallable, Category="Combat")
	virtual void ChangedOnTile(const bool IsOnField);

	UFUNCTION(BlueprintCallable, Category="Combat")
	bool IsOnField() const { return bIsOnField; }

	UFUNCTION(BlueprintCallable, Category="Combat")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintCallable, Category="Combat")
	bool IsStunned() const { return bIsStunned; }
	
	UFUNCTION(BlueprintCallable)
	virtual void Die();

	UFUNCTION(BlueprintCallable)
	void OnDeathAnimCompleted();

	UFUNCTION(BlueprintCallable)
	void CombatWin(APCPlayerState* TargetPS);

	UFUNCTION(BlueprintCallable)
	void CombatDraw(APCPlayerState* TargetPS);

	UFUNCTION(BlueprintCallable, Category="Combat")
	bool IsCombatWin() const { return bIsCombatWin; }
	
protected:
	virtual void OnGameStateChanged(const FGameplayTag& NewStateTag);
	FDelegateHandle GameStateChangedHandle;
	
	UPROPERTY()
	TObjectPtr<APCCombatBoard> OnCombatBoard;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category="Combat")
	bool bIsOnField = false;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_IsCombatWin, Category="Combat")
	bool bIsCombatWin = false;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_IsDead, Category="Combat")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Category="Combat")
	bool bIsStunned = false;

	FVector DeadZone = FVector(25000.f, 10000.f, 50.f);
	
	FDelegateHandle DeadHandle;
	FDelegateHandle StunHandle;

	UFUNCTION()
	void OnRep_IsCombatWin();
	
	UFUNCTION()
	void OnRep_IsDead();
	
	void OnUnitStateTagChanged(const FGameplayTag Tag, int32 NewCount);
	
public:
	UPROPERTY()
	FOnUnitDied OnUnitDied;
};
