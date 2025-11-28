// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/ZombieEnemy.h"

// Sets default values
AZombieEnemy::AZombieEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AZombieEnemy::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AZombieEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AZombieEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

float AZombieEnemy::TryPlayMontage(UAnimMontage* Montage, float PlayRate, FName StartSessionName)
{
	float Duration = PlayAnimMontage(Montage, PlayRate, StartSessionName);
	//서버에서 검사
	if (Duration > 0.f)
	{
		MultiPlayMontage(Montage, PlayRate, StartSessionName);
	}
	return Duration;
}

void AZombieEnemy::MultiPlayMontage_Implementation(UAnimMontage* Montage, float PlayRate, FName StartSessionName)
{
	//중복실행방지
	if (HasAuthority())
	{
		return;
	}
	PlayAnimMontage(Montage, PlayRate, StartSessionName);
	return;
}

void AZombieEnemy::TryStopMontage(UAnimMontage* Montage)
{
	MultiStopMontage(Montage);
}

void AZombieEnemy::MultiStopMontage_Implementation(UAnimMontage* Montage)
{
	StopAnimMontage(Montage);
	return;
}
