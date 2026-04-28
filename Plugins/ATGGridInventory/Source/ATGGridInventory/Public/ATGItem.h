// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATGItem.generated.h"

class UATGPickupComponent;

UCLASS()
class ATGGRIDINVENTORY_API AATGItem : public AActor
{
	GENERATED_BODY()

public:
	AATGItem();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UATGPickupComponent> PickupComp;

public:
	FORCEINLINE UATGPickupComponent* GetPickupComp() const { return PickupComp; }
};
