// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZombieEnemy.generated.h"

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	None		= 0		UMETA(DisplayName = "None"),
	Normal		= 10	UMETA(DisplayName = "Normal"),
	Chase		= 20	UMETA(DisplayName = "Chase"),
	Battle		= 30	UMETA(DisplayName = "Battle"),
	Death		= 40	UMETA(DisplayName = "Death"),
};

UCLASS()
class PROJECTA_API AZombieEnemy : public ACharacter
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

	EEnemyState EnemyState = EEnemyState::Normal;
};
