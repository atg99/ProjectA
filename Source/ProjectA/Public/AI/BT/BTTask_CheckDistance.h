// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "ATGEnum.h"
#include "BTTask_CheckDistance.generated.h"

UENUM(BlueprintType)
enum class ETargetDistanceState : uint8
{
	None = 0	UMETA(DisplayName = "None"),
	Less = 10	UMETA(DisplayName = "Less"),
	More = 20	UMETA(DisplayName = "More"),
};

/**
 * 
 */
UCLASS()
class PROJECTA_API UBTTask_CheckDistance : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:

	UBTTask_CheckDistance();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition")
	float TargetDistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition")
	ETargetDistanceState Condition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition")
	EMonsterState TargetState;
};
