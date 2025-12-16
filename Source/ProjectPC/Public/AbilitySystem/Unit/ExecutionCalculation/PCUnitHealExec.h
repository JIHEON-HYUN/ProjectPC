// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseGameplayTags.h"
#include "GameplayEffectExecutionCalculation.h"
#include "PCUnitHealExec.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTPC_API UPCUnitHealExec : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	
protected:
	FGameplayTag HealCallerTag = GameplayEffectTags::GE_Caller_Heal;
	
	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
