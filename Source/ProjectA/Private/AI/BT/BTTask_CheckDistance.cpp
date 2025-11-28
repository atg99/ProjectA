// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BTTask_CheckDistance.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/ZombieEnemy.h"
#include "AIController.h"
#include "AI/BaseAIController.h"


UBTTask_CheckDistance::UBTTask_CheckDistance()
{
	NodeName = TEXT("BTT_CheckDistance");
}

EBTNodeResult::Type UBTTask_CheckDistance::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	
	AActor* Player = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(GetSelectedBlackboardKey()));
	ABaseAIController* BaseAIC = Cast<ABaseAIController>(OwnerComp.GetAIOwner());
	APawn* Self = Cast<APawn>(OwnerComp.GetAIOwner()->GetPawn());

	float Dis = FVector::Distance(Self->GetActorLocation(), Player->GetActorLocation());

	if (IsValid(Player) && IsValid(BaseAIC), IsValid(Self))
	{
		switch (Condition)
		{
		case ETargetDistanceState::More:
		{
			if (Dis > TargetDistance)
			{
				BaseAIC->SetState(TargetState);
			}
			break;
		}
		case ETargetDistanceState::Less:
		{
			if (Dis <= TargetDistance)
			{
				BaseAIC->SetState(TargetState);
			}
			break;
		}
		}

		return EBTNodeResult::Succeeded;
	}
	

	return EBTNodeResult::Succeeded;
}
