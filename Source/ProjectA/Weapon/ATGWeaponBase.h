// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Public/ATGInterface.h"
#include "../Public/ATGEnum.h"
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USkeletalMeshComponent> Mesh;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TSubclassOf<ABulletBase> BulletTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SocketName = TEXT("HandGrip_R");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	int32 MaxBulletCount = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	int32 CurrentBulletCount = 100;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	TObjectPtr<UAnimMontage> FireMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	TObjectPtr<UAnimMontage> ReloadMontage;

protected:

	virtual void PlayerInteract(FInteractionData& InteractionData) override;

};
