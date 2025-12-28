// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/NotifyState_AddGameplayTag.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

FString UNotifyState_AddGameplayTag::GetNotifyName_Implementation() const
{
	return FString("AddGameplayTag");
}

void UNotifyState_AddGameplayTag::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner());
		if (ASC)
		{
			// 복제되지 않는 로컬 태그
			ASC->AddLooseGameplayTag(GameplayTag);
		}
	}
}

void UNotifyState_AddGameplayTag::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner());
		if (ASC)
		{
			ASC->RemoveLooseGameplayTag(GameplayTag);
		}
	}
}
