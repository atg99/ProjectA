// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "NotifyState_MeleeAttack.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTA_API UNotifyState_MeleeAttack : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	TArray<AActor> HitActors;

	class IMeleeWeaponInterface* GetWeaponInterface(USkeletalMeshComponent* MeshComp);
};
