// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryTypes.h"
#include "ATGInventoryGirdWidget.generated.h"

class UButton;
class UGridPanel;
class UImage;
class UGridSlot;
class UATGInventoryItemWidget;
class UATGInventoryComponent;
class UTexture2D;
class UATGStackSplitWidget;

/**
 * 
 */
UCLASS()
class PROJECTA_API UATGInventoryGirdWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	UButton* Btn_Sort = nullptr;

	// 디자이너에서 GridPanel 바인드 (Named Slot: GridPanel)
	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	UGridPanel* GridPanel = nullptr;

	// 픽셀 단위 셀 크기 & 패딩
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout")
	int32 CellSize = 64;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout")
	int32 CellPadding = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Skin")
	FLinearColor BGColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.f);

	FLinearColor DefaultColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Skin")
	FLinearColor CheckFalseColor = { 0.5f, 0, 0, 0.5f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Skin")
	FLinearColor CheckTrueColor = { 0, 0.5f, 0, 0.5f };

	// 셀 배경 텍스처(옵션)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Skin")
	UTexture2D* DefaultCellBg = nullptr;

	UATGStackSplitWidget* SplitUI;

	FIntPoint PrevCell;

	// 인벤토리 소스
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	UATGInventoryComponent* InventoryComp = nullptr;

protected:
	// Id -> ItemWidget 맵(부분 갱신용)
	UPROPERTY(Transient)
	TMap<int32, TWeakObjectPtr<UATGInventoryItemWidget>> IdToWidget;

	//preview 전용
	//UPROPERTY(Transient)
	TMap<int32, TArray<TWeakObjectPtr<UATGInventoryItemWidget>>> PreviewIdToWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UATGInventoryItemWidget> InventoryItemWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UATGStackSplitWidget> StackSplitWidgetClass;

public:
	// 소유자 기준으로 인벤토리 찾아 자동 초기화(또는 BP에서 직접 Set 해도 됨)
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeFromOwner();


	// 전체 리빌드
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RebuildAll();
	void BindInventoryComp();

	virtual bool NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnDragEnter(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	// 델리게이트 핸들러
	//UFUNCTION() void HandleInitInventoryComp(UATGInventoryComponent* GetInventoryComponent);

	UFUNCTION() void HandleItemAdded(int32 EntryId);
	UFUNCTION() void HandleItemChanged(int32 EntryId);
	UFUNCTION() void HandleItemRemoved(int32 EntryId);
	UFUNCTION() void HandleItemRotated(int32 EntryId);

	UFUNCTION() void HandleItemPreAdded(FInventoryEntry PreE);
	UFUNCTION() void HandleItemPreChanged(FInventoryEntry PreE);
	UFUNCTION() void HandleItemPreRemoved(int32 PreEId);


	// 헬퍼
	const FInventoryEntry* FindEntryById(int32 EntryId) const;
	UATGInventoryItemWidget* CreateItemWidget(const FInventoryEntry& E);
	void UpdateItemSlot(UATGInventoryItemWidget* W, const FInventoryEntry& E);
	FIntPoint CellFromLocal(const FVector2D& Local) const;
	bool CheckIsOutGrid(const FVector2D& Local) const;
	void DoNativeOnDrop(UATGInventoryItemWidget* Dragged, FVector2D Screen);
	void DoNativeOnDrop(UATGInventoryItemWidget* Dragged, FVector2D Screen, int32 SplitNum);
	// 셀 배경 생성
	void BuildCellBackground();

	bool bIsDragLeave = false;

protected:
	//드래그관련
	UDragDropOperation* Operation;
	bool bIsRotate = false;

	void SetAllGridDefaultColor();

	UFUNCTION()
	void OnSortBtnClicked();
};
