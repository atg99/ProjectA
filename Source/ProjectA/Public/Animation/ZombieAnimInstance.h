// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AI/ZombieEnemy.h"
#include "ZombieAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTA_API UZombieAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement)
	FVector Velocity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement)
	float GroundSpeed;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement)
	//float Direction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement)
	bool ShouldMove;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Movement)
	bool bIsDie;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Movement)
	EEnemyState EnemyState = EEnemyState::None;

};
