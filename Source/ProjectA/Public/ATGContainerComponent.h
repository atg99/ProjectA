// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ATGInterface.h"
#include "ATGEnum.h"
#include "InventoryTypes.h"
#include "ATGInventoryOwnerInterface.h"
#include "ATGContainerComponent.generated.h"

class AATGItem;

USTRUCT(BlueprintType)
struct FContainerItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UATGItemData> ItemDef;

	UPROPERTY(EditAnywhere)
	int32 Quantity = 1;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnContainerGridEvent, int32, EntryId);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTA_API UATGContainerComponent : public UActorComponent, public IATGInterface, public IATGInventoryOwnerInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UATGContainerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void OnRegister() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	// FastArray 
	UPROPERTY(Replicated)
	FInventoryGrid ContainerInventory;

	UPROPERTY(EditAnywhere, meta=(ToolTip = "Init Container Value"))
	TArray<FContainerItem> ContainerItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_GridSize)
	FIntPoint GridSize = FIntPoint(10, 10);

	UFUNCTION()
	void OnRep_GridSize();

	//UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly)
	//int32 GridSize = 10;

	//UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly)
	//int32 GridHeight = 10;

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	EInteractionType InteractionType = EInteractionType::ItemGridBox;

	FORCEINLINE FInventoryGrid& GetContainerInventory() { return ContainerInventory; }

	// Blueprint Helper
	UFUNCTION(BlueprintCallable)
	const FInventoryEntry& GetById(int32 EntryId);

public:

	UFUNCTION(BlueprintCallable)
	virtual void IncreaseGridSize(int32 W, int32 H) override;

	UFUNCTION(BlueprintCallable)
	virtual void DecreaseGridSize(int32 W, int32 H) override;

	UFUNCTION(BlueprintCallable)
	virtual void SetGridSize(int32 W, int32 H) override;

	UFUNCTION(BlueprintCallable)
	virtual FIntPoint GetGridSize() override;

	UPROPERTY(BlueprintAssignable)
	FOnContainerGridEvent OnContainerAdded;

	UPROPERTY(BlueprintAssignable)
	FOnContainerGridEvent OnContainerRemoved;

	UPROPERTY(BlueprintAssignable)
	FOnContainerGridEvent OnContainerChanged;

	UPROPERTY(BlueprintAssignable)
	FOnContainerGridEvent OnRebuildAll;


	//UPROPERTY(BlueprintAssignable)
	//FOnContainerGridEvent OnContainerRotated;

	void InitContainerItem();

	////RPCs
	//UFUNCTION(BlueprintCallable)
	//virtual const struct FInventoryGrid& GetInventory() override { return };

protected:

	//Interact
	virtual void PlayerInteract(FInteractionData& InteractionData) override;

	virtual void ItemRemoved(int32 EntryId) override;

	virtual void ItemAdded(int32 EntryId) override;

	virtual void ItemChanged(int32 EntryId) override;

	virtual void InventoryForceNetUpdate() override;

	virtual bool IsLocallyOwned() override;

	virtual const TArray<FInventoryEntry>& GetEntries() override { return ContainerInventory.Entries; }

	virtual int32 GetGridWidth() const override { return ContainerInventory.GridWidth; }

	virtual int32 GetGridHeight() const override { return ContainerInventory.GridHeight; }

	virtual const FInventoryGrid& GetInventory() override { return ContainerInventory; }

	virtual bool CheckCanMove(int32 StartX, int32 StartY, int32 W, int32 H, int32 IgnoreId = -1) override;

	virtual void TryMoveOrSwapClient(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate) override;

	virtual void TrySplitStack(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate, int32 SplitNum) override;

	virtual void TryDropItem(int32 EntryId, int32 SplitNum = -1) override;

	virtual void TryAddItemAt(TScriptInterface<IATGInventoryOwnerInterface> Inven, int32 OtherGridId, TSoftObjectPtr<class UATGItemData> ItemDef, int32 InQty, int32 X, int32 Y, bool bRotate = false) override;

	UFUNCTION()
	virtual void TrySortByItemId() override;

	UFUNCTION(Server, Reliable)
	void ServerSortByItemId();

	TArray<int32> AddItemAuto(FClientAddRequest& ClientAddRequest);

	UFUNCTION(Server, Reliable)
	void ServerAddItemAt(FClientAddRequest ClientAddRequest, int32 OtherGridId, const TScriptInterface<IATGInventoryOwnerInterface>& Inven);

	UFUNCTION(Server, Reliable)
	void ServerDropItem(int32 EntryId, int32 SplitNum = -1);

	UFUNCTION(Server, Reliable)
	void ServerSpawnItem(int32 EntryId, int32 SplitNum = -1);

	UFUNCTION(Server, Reliable)
	void ServerRemoveItem(int32 EntryId);

	virtual void TryHandleTransItemResult(int32 EntryId, int32 RemoveQty = -1) override;

	UFUNCTION(Server, Reliable)
	void ServerHandleTransItemResult(int32 EntryId, int32 RemoveQty = -1);

	UFUNCTION(Server, Reliable)
	void ServerMoveOrSwap(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate);

	UFUNCTION(Server, Reliable)
	void ServerSplitStack(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate, int32 SplitNum);
	//virtual void TryDropItem(int32 EntryId, int32 SplitNum = -1) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TSubclassOf<AATGItem> ItemBPClass;

};
