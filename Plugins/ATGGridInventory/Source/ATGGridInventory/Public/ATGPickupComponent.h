// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ATGInventoryInterface.h"
#include "ATGPickupComponent.generated.h"

class UATGItemData;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ATGGRIDINVENTORY_API UATGPickupComponent : public UActorComponent, public IATGInterface
{
	GENERATED_BODY()

public:
	UATGPickupComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void PlayerInteract(FInteractionData& InteractionData) override;

public:
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UATGItemData> ItemDef;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 ItemQty = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Item")
	EInteractionType InteractionType = EInteractionType::PickUpItem;

	void DecreaseQty(int32 Amount);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(Replicated)
	int32 TestInt = 0;

	void SetItemMesh();
};
