// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryTypes.h"
#include "ATGInventoryComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGridEvent, int32, EntryId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGridPreEvent, FInventoryEntry, PreE);

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

	UPROPERTY(BlueprintAssignable)
	FOnGridPreEvent OnItemPreAdded;

	UPROPERTY(BlueprintAssignable)
	FOnGridEvent OnItemPreRemoved;

	// Server RPCs
	UFUNCTION(Server, Reliable)
	void ServerAddItemAuto(FClientAddRequest ClientAddRequest);

	UFUNCTION()
	int32 AddItemAuto(const FClientAddRequest& ClientAddRequest);

	UFUNCTION(Server, Reliable)
	void ServerAddItemAt(UATGItemData* ItemDef, int32 Quantity, int32 X, int32 Y, bool bRotated);

	UFUNCTION(Server, Reliable)
	void ServerMoveOrSwap(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate);

	UFUNCTION(Server, Reliable)
	void ServerRotateItem(int32 EntryId);

	UFUNCTION(Server, Reliable)
	void ServerRemoveItem(int32 EntryId);

	//Client CallBack
	UFUNCTION(Client, Reliable)
	void ClientAddItemResult(FInventoryChangeResult Result);

	UFUNCTION(Client, Reliable)
	void ClientMoveResult(const FInventoryChangeResult& Result);

	// Client Preview + ServerRPC
	UFUNCTION()
	void TryPickupClient(TSoftObjectPtr<UATGItemData> ItemDef, int32 Quantity);

	UFUNCTION()
	void TryMoveOrSwapClient(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate);

	// Blueprint Helpers
	UFUNCTION(BlueprintCallable, Category = "Inventory|Grid")
	const TArray<FInventoryEntry>& GetEntries() const { return Inventory.Entries; }

	UFUNCTION(BlueprintCallable, Category = "Inventory|Grid")
	int32 GetGridWidth() const { return Inventory.GridWidth; }

	UFUNCTION(BlueprintCallable, Category = "Inventory|Grid")
	int32 GetGridHeight() const { return Inventory.GridHeight; }

	bool IsHasAuthority();

	bool IsLocallyOwned() const;

protected:

	//TMap<int32 /*ServerEntryId*/, int32 /*PredictionId*/> PendingServerToPred;
	UFUNCTION()
	void HandleReplicatedAdd(int32 EntryId);

	//UFUNCTION()
	//void HandleReplicatedChange(int32 EntryId);

	int32 LocalPred = -1;

	////server API 
	//UFUNCTION(Server, Reliable)
	//void ServerAddItem(UATGItemData* ItemDef, int32 Quantity);

	//UFUNCTION(Server, Reliable)
	//void ServerRemoveItem(UATGItemData* ItemDef, int32 Quantity);

	//UFUNCTION(BlueprintCallable, Category = "Inventory")
	//const TArray<FInventoryEntry>& GetEntries() const { return Inventory.Entries; }

};
