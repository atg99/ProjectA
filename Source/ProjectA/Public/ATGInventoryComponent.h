// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryTypes.h"
#include "ATGInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGridEvent, int32, EntryId);

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

	// FastArray 
	UPROPERTY(EditAnywhere ,Replicated)
	FInventoryGrid Inventory;

public:
	// client UI
	UPROPERTY(BlueprintAssignable) 
	FOnGridEvent OnItemAdded;
	UPROPERTY(BlueprintAssignable) 
	FOnGridEvent OnItemRemoved;
	UPROPERTY(BlueprintAssignable) 
	FOnGridEvent OnItemChanged;

	UPROPERTY(BlueprintAssignable)
	FOnGridEvent OnItemRotated;

	// Server RPCs
	UFUNCTION(Server, Reliable)
	void ServerAddItemAuto(UATGItemData* ItemDef, int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerAddItemAt(UATGItemData* ItemDef, int32 Quantity, int32 X, int32 Y, bool bRotated);

	UFUNCTION(Server, Reliable)
	void ServerMoveOrSwap(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate);

	UFUNCTION(Server, Reliable)
	void ServerRotateItem(int32 EntryId);

	UFUNCTION(Server, Reliable)
	void ServerRemoveItem(int32 EntryId);

	// Blueprint Helpers
	UFUNCTION(BlueprintCallable, Category = "Inventory|Grid")
	const TArray<FInventoryEntry>& GetEntries() const { return Inventory.Entries; }

	UFUNCTION(BlueprintCallable, Category = "Inventory|Grid")
	int32 GetGridWidth() const { return Inventory.GridWidth; }

	UFUNCTION(BlueprintCallable, Category = "Inventory|Grid")
	int32 GetGridHeight() const { return Inventory.GridHeight; }

	////server API 
	//UFUNCTION(Server, Reliable)
	//void ServerAddItem(UATGItemData* ItemDef, int32 Quantity);

	//UFUNCTION(Server, Reliable)
	//void ServerRemoveItem(UATGItemData* ItemDef, int32 Quantity);

	//UFUNCTION(BlueprintCallable, Category = "Inventory")
	//const TArray<FInventoryEntry>& GetEntries() const { return Inventory.Entries; }

};
