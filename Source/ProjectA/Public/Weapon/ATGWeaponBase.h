// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Public/ATGEnum.h"
#include "ATGWeaponBase.generated.h"

class UStaticMeshComponent;
class UATGWeaponData;

UCLASS()
class PROJECTA_API AATGWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AATGWeaponBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USkeletalMeshComponent> Mesh;

public:

	UPROPERTY(ReplicatedUsing = OnRep_WeaponData, BlueprintReadOnly)
	UATGWeaponData* WeaponData;

	UFUNCTION()
	virtual void OnRep_WeaponData();

};

