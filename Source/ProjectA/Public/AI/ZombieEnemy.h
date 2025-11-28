// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interface/ATGBTInterface.h"
#include "ZombieEnemy.generated.h"

UCLASS()
class PROJECTA_API AZombieEnemy : public ACharacter, public IATGBTInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AZombieEnemy();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual float TryPlayMontage(UAnimMontage* Montage, float PlayRate = 1.f, FName StartSessionName = NAME_None) override;

	UFUNCTION(NetMulticast, Reliable)
	void MultiPlayMontage(UAnimMontage* Montage, float PlayRate = 1.f, FName StartSessionName = NAME_None);

	virtual void TryStopMontage(UAnimMontage* Montage) override;

	UFUNCTION(NetMulticast, Reliable)
	void MultiStopMontage(UAnimMontage* Montage);
};
