// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/ZombieAnimInstance.h"
#include "AI/ZombieEnemy.h"
#include <AI/BaseAIController.h>

void UZombieAnimInstance::NativeInitializeAnimation()
{
	//Character = Cast<ACharacter>(TryGetPawnOwner());
}

void UZombieAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	AZombieEnemy* Enemy = Cast<AZombieEnemy>(TryGetPawnOwner());

	if (Enemy)
	{
		Velocity = Enemy->GetVelocity();
		GroundSpeed = Velocity.Size2D();
		ShouldMove = (GroundSpeed > 3.0f);
		ABaseAIController* AIC = Cast<ABaseAIController>(Enemy->GetController());
		if (AIC)
		{
			MonsterState = AIC->MonsterState;
		}
	}
}
