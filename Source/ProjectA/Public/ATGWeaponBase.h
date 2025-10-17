// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATGInterface.h"
#include "ATGWeaponBase.generated.h"

class UStaticMeshComponent;

UCLASS()
class PROJECTA_API AATGWeaponBase : public AActor, public IATGInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AATGWeaponBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* StaticMeshComp;

protected:

	virtual void PlayerInteract(AActor*& Weapon) override;

};
