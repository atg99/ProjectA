// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/NotifyState_MeleeCombo.h"
#include "MeleeComponent.h"

FString UNotifyState_MeleeCombo::GetNotifyName_Implementation() const
{
	return FString("ComboAttackRange");
}

void UNotifyState_MeleeCombo::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (UMeleeComponent* MeleeComponent = Cast<UMeleeComponent>(MeshComp->GetOwner()->GetComponentByClass(UMeleeComponent::StaticClass())))
	{
		MeleeComponent->bIsCanCombo = true;
	}

	return;
}

void UNotifyState_MeleeCombo::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (UMeleeComponent* MeleeComponent = Cast<UMeleeComponent>(MeshComp->GetOwner()->GetComponentByClass(UMeleeComponent::StaticClass())))
	{
		MeleeComponent->bIsCanCombo = false;
	}

	return;
}
