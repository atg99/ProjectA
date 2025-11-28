// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_PlayMontage.generated.h"

/**
 * 
 */
class UAnimMontage;
class UBehaviorTreeComponent;

UCLASS()
class PROJECTA_API UBTTask_PlayMontage : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:

	UBTTask_PlayMontage();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition")
	UAnimMontage* Montage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition")
	FName StartSessionName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition")
	float PlayRate = 1.f;

	UFUNCTION()
	void HandleMontageEnded(UAnimMontage* InMontage, bool bInterrupted);

	UBehaviorTreeComponent* CachedOwnerComp = nullptr;
	
};
