// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BT/BTTask_PlayMontage.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Interface/ATGBTInterface.h"
#include "Animation/AnimMontage.h"

UBTTask_PlayMontage::UBTTask_PlayMontage()
{	
	bCreateNodeInstance = true; //AI마다 인스턴스 생성
	NodeName = TEXT("BTT_PlayMontage");
}

EBTNodeResult::Type UBTTask_PlayMontage::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ACharacter* Self = Cast<ACharacter>(OwnerComp.GetAIOwner()->GetPawn());
	IATGBTInterface* BTInterface = Cast<IATGBTInterface>(Self);
	UAnimInstance* AnimInst = Self->GetMesh()->GetAnimInstance();
	
	if (!IsValid(Self) || !IsValid(AnimInst) || !Montage || !BTInterface)
	{
		return EBTNodeResult::Failed;
	}

	CachedOwnerComp = &OwnerComp;
	float Duration = 0.f;
	if (StartSessionName.IsNone())
	{
		Duration = BTInterface->TryPlayMontage(Montage, PlayRate);
		//Duration = Self->PlayAnimMontage(Montage, PlayRate);
	}
	else
	{
		Duration = BTInterface->TryPlayMontage(Montage, PlayRate, StartSessionName);
	}

	if (Duration <= 0.f)
	{
		return EBTNodeResult::Failed;
	}

	//재생시킨 몽타주 바인딩

	FOnMontageEnded EndDelegate;
	EndDelegate.Unbind();
	EndDelegate.BindUObject(this, &UBTTask_PlayMontage::HandleMontageEnded);
	AnimInst->Montage_SetEndDelegate(EndDelegate, Montage);

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_PlayMontage::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ACharacter* Self = Cast<ACharacter>(OwnerComp.GetAIOwner()->GetPawn());
	IATGBTInterface* BTInterface = Cast<IATGBTInterface>(Self);
	if (IsValid(Self) && BTInterface)
	{
		UAnimInstance* AnimInst = Self->GetMesh()->GetAnimInstance();
		if (IsValid(AnimInst) && Montage)
		{
			//Unbind
			FOnMontageEnded EmptyDelegate;
			AnimInst->Montage_SetEndDelegate(EmptyDelegate, Montage);

			// 델리게이트를 먼저 해제한 후 몽타주를 정지시켜야 안전
			BTInterface->TryStopMontage(Montage);
		}
	}

	return Super::AbortTask(OwnerComp, NodeMemory);
}

void UBTTask_PlayMontage::HandleMontageEnded(UAnimMontage* InMontage, bool bInterrupted)
{
	UE_LOG(LogTemp, Log, TEXT("UBTTask_PlayMontage::HandleMontageEnded"));
	if (bInterrupted)
	{
		FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Failed);
	}
	else
	{
		FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
	}

	ACharacter* Self = Cast<ACharacter>(CachedOwnerComp->GetAIOwner()->GetPawn());
	if (Self && Self->GetMesh()->GetAnimInstance())
	{
		//Unbind
		FOnMontageEnded EndDelegate;
		Self->GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate,InMontage);
	}
}
