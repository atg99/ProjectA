// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ATGInterface.h"
#include "ATGEnum.h"
#include "ATGPickupComponent.generated.h"

class UATGItemData;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTA_API UATGPickupComponent : public UActorComponent, public IATGInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UATGPickupComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	virtual void PlayerInteract(FInteractionData& InteractionData) override;


public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UATGItemData> ItemDef;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 ItemQty = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EInteractionType InteractionType = EInteractionType::Inventory;

protected:

	void SetItemMesh();
};
