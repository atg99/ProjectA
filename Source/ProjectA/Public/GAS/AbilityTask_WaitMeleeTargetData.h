// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitMeleeTargetData.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMeleeTargetDataEvent, const FGameplayAbilityTargetDataHandle&, Data);

UCLASS()
class PROJECTA_API UAbilityTask_WaitMeleeTargetData : public UAbilityTask
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_WaitMeleeTargetData* WaitMeleeTargetData(UGameplayAbility* OwningAbility, FGameplayTag EventTag);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

	UPROPERTY(BlueprintAssignable)
	FMeleeTargetDataEvent ValidData;

protected:
	FGameplayTag TriggerTag;

	//등록한 델리게이트를 해제하기 위한 핸들
	FDelegateHandle DelegateHandle;

	//Client 로컬 이벤트를 수신했을 때
	void OnLocalGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);

	//Server 클라이언트로부터 TargetData가 도착했을 때
	void OnTargetDataReplicated(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ActivationTag);
	
};
