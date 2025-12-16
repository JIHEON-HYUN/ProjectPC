// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "PCPreloadHeroActor.generated.h"

UCLASS()
class PROJECTPC_API APCPreloadHeroActor : public AActor
{
	GENERATED_BODY()

public:	
	APCPreloadHeroActor();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

public:
	void SetUnitTag(const FGameplayTag& InUnitTag) { if (HasAuthority()) { UnitTag = InUnitTag; } }
	void SetUnitLevel(const int32 InUnitLevel) { UnitLevel = InUnitLevel; }
	USkeletalMeshComponent* GetMesh() const { return MeshComp; }
	
protected:
	UPROPERTY(VisibleAnywhere, Category="Component")
	TObjectPtr<USkeletalMeshComponent> MeshComp;

	UPROPERTY(ReplicatedUsing=OnRep_UnitTag)
	FGameplayTag UnitTag;

	UPROPERTY(Replicated)
	int32 UnitLevel = 1;
	
	UFUNCTION()
	void OnRep_UnitTag() const;

	void ApplyVisualsForUnitTag() const;
};
