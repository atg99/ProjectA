// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BTTask_SetRandomPoint.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"

UBTTask_SetRandomPoint::UBTTask_SetRandomPoint()
{
	NodeName = "SetRandomPoint";
}

EBTNodeResult::Type UBTTask_SetRandomPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *OwnerComp.GetOwner()->GetName());

	return EBTNodeResult::Succeeded;
}
