// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryTypes.h"
#include "ATGInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemEvent, const FInventoryEntry&, Entry);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTA_API UATGInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UATGInventoryComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

public:
	// FastArray 
	UPROPERTY(Replicated)
	FInventoryList Inventory;

	// client UI
	UPROPERTY(BlueprintAssignable) 
	FOnItemEvent OnItemAdded;
	UPROPERTY(BlueprintAssignable) 
	FOnItemEvent OnItemRemoved;
	UPROPERTY(BlueprintAssignable) 
	FOnItemEvent OnItemChanged;

	//server API 
	UFUNCTION(Server, Reliable)
	void ServerAddItem(UATGItemData* ItemDef, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerRemoveItem(UATGItemData* ItemDef, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	const TArray<FInventoryEntry>& GetEntries() const { return Inventory.Entries; }

};
