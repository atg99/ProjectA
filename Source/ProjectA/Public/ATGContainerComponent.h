// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ATGInterface.h"
#include "ATGEnum.h"
#include "InventoryTypes.h"
#include "ATGContainerComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTA_API UATGContainerComponent : public UActorComponent, public IATGInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UATGContainerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	// FastArray 
	UPROPERTY(EditAnywhere, Replicated)
	FInventoryGrid ContainerInventory;

	virtual void PlayerInteract(FInteractionData& InteractionData) override;

public:

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	EInteractionType InteractionType = EInteractionType::ItemGridBox;

	FORCEINLINE FInventoryGrid& GetContainerInventory() { return ContainerInventory; }

		
};
