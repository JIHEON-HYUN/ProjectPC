// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Unit/ExecutionCalculation/PCUnitHealExec.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Unit/AttributeSet/PCUnitAttributeSet.h"

void UPCUnitHealExec::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
                                             FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// 힐량 계산, 적용은 서버에서만 실행
	if (!SourceASC || !SourceASC->GetOwner()->HasAuthority())
		return;

	float BaseHeal = Spec.GetSetByCallerMagnitude(HealCallerTag, 0.f);
	if (BaseHeal <= 0.f)
	{
		return; // 힐량 없음
	}
	
	// Health에 양수로 적용
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		UPCUnitAttributeSet::GetCurrentHealthAttribute(),
		EGameplayModOp::Additive,
		BaseHeal));

	FGameplayCueParameters CueParams;
	CueParams.EffectContext = Spec.GetEffectContext();
	CueParams.RawMagnitude = BaseHeal;
	CueParams.NormalizedMagnitude = 0.f;
	CueParams.Instigator = SourceASC->GetAvatarActor();
	CueParams.AggregatedSourceTags.AddTag(UnitGameplayTags::Unit_CombatText_Type_Heal);
	
	SourceASC->ExecuteGameplayCue(GameplayCueTags::GameplayCue_UI_Unit_CombatText, CueParams);
}
