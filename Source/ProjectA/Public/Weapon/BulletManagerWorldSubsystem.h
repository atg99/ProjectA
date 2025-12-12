// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ATGEnum.h"
#include "BulletManagerWorldSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTA_API UBulletManagerWorldSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	
public:
	UBulletManagerWorldSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;


	virtual void Tick(float DeltaTime) override;

	virtual TStatId GetStatId() const override;

	TArray<FBullet> ActiveBullets;

	virtual bool IsTickable() const override { return false; }

	FTimerHandle TimerHandle_Loop;

	void SimulateBullets();

	float SimulateInterval = 0.1f;
};
