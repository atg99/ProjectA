// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATGItem.generated.h"

class UATGPickupComponent;
class UATGItemData;
class UStaticMeshComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ItemMeshComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Mesh")
	bool bApplyItemDataMesh = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Mesh", meta = (EditCondition = "bApplyItemDataMesh"))
	bool bSimulatePhysicsAfterMeshApplied = false;

public:
	FORCEINLINE UATGPickupComponent* GetPickupComp() const { return PickupComp; }
	FORCEINLINE UStaticMeshComponent* GetItemMeshComp() const { return ItemMeshComp; }

	UFUNCTION(BlueprintCallable, Category = "Item|Mesh")
	void ApplyItemDataMesh(UATGItemData* ItemData);
};
