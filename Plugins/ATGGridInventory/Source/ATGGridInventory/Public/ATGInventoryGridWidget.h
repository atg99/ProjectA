// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ATGInventoryOwnerInterface.h"
#include "InventoryTypes.h"
#include "ATGInventoryGridWidget.generated.h"

class UButton;
class UGridPanel;
class UImage;
class UTexture2D;
class UDragDropOperation;
class UATGContainerComponent;
class UATGInventoryComponent;
class UATGInventoryItemWidget;
class UATGStackSplitWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGridRebuild, int32, ItemNum);

UCLASS()
class ATGGRIDINVENTORY_API UATGInventoryGridWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UButton> Btn_Sort = nullptr;

	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UGridPanel> GridPanel = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout")
	int32 CellSize = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout")
	int32 CellPadding = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Skin")
	FLinearColor BGColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.f);

	FLinearColor DefaultColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Skin")
	FLinearColor CheckFalseColor = FLinearColor(0.5f, 0.f, 0.f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Skin")
	FLinearColor CheckTrueColor = FLinearColor(0.f, 0.5f, 0.f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Skin")
	TObjectPtr<UTexture2D> DefaultCellBg = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanDrag = true;

	UPROPERTY()
	TObjectPtr<UATGStackSplitWidget> SplitUI = nullptr;

	FIntPoint PrevCell = FIntPoint(-1, -1);

	UPROPERTY(BlueprintReadWrite)
	TScriptInterface<IATGInventoryOwnerInterface> Inven = nullptr;

	UPROPERTY(BlueprintAssignable)
	FOnGridRebuild OnGridRebuild;

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeFromOwner();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void RebuildAll();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void BindInventoryComp();

	UFUNCTION(BlueprintCallable)
	int32 GetDBIdByEntryId(int32 ItemEntryId);

	virtual bool NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

protected:
	UPROPERTY(Transient)
	TMap<int32, TWeakObjectPtr<UATGInventoryItemWidget>> IdToWidget;

	TMap<int32, TArray<TWeakObjectPtr<UATGInventoryItemWidget>>> PreviewIdToWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UATGInventoryItemWidget> InventoryItemWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UATGStackSplitWidget> StackSplitWidgetClass;

	virtual void NativeConstruct() override;
	virtual void NativeOnDragEnter(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UFUNCTION()
	void HandleItemAdded(int32 EntryId);

	UFUNCTION()
	void HandleItemChanged(int32 EntryId);

	UFUNCTION()
	void HandleItemRemoved(int32 EntryId);

	UFUNCTION()
	void HandleRebuildAll(int32 EntryId);

	UFUNCTION()
	void HandleItemPreAdded(FInventoryEntry PreE);

	UFUNCTION()
	void HandleItemPreChanged(FInventoryEntry PreE);

	UFUNCTION()
	void HandleItemPreRemoved(int32 PreEId);

	const FInventoryEntry* FindEntryById(int32 EntryId) const;
	UATGInventoryItemWidget* CreateItemWidget(const FInventoryEntry& E);
	void UpdateItemSlot(UATGInventoryItemWidget* W, const FInventoryEntry& E);
	FIntPoint CellFromLocal(const FVector2D& Local) const;
	bool CheckIsOutGrid(const FVector2D& Local) const;
	void DoNativeOnDrop(UDragDropOperation* InOperation, UATGInventoryItemWidget* Dragged, FVector2D Screen);
	void DoNativeOnDrop(UDragDropOperation* InOperation, UATGInventoryItemWidget* Dragged, FVector2D Screen, int32 SplitNum);

	virtual bool CheckIsFromOther(UATGInventoryItemWidget* Dragged);
	virtual void BuildCellBackground();
	virtual void HandleIncomingItem(UDragDropOperation* InOperation, UATGInventoryItemWidget* InDragged, FVector2D Screen);

	bool bIsDragLeave = true;
	TObjectPtr<UDragDropOperation> Operation = nullptr;

	void SetAllGridDefault();

	UFUNCTION()
	void OnSortBtnClicked();
};
