// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryTypes.h"
#include "ATGInventoryGirdWidget.generated.h"

class UGridPanel;
class UImage;
class UGridSlot;
class UATGInventoryItemWidget;
class UATGInventoryComponent;
class UTexture2D;

/**
 * 
 */
UCLASS()
class PROJECTA_API UATGInventoryGirdWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 디자이너에서 GridPanel 바인드 (Named Slot: GridPanel)
	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	UGridPanel* GridPanel = nullptr;

	// 픽셀 단위 셀 크기 & 패딩
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout")
	int32 CellSize = 64;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout")
	int32 CellPadding = 1;


	// 셀 배경 텍스처(옵션)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Skin")
	UTexture2D* DefaultCellBg = nullptr;


	// 인벤토리 소스
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	UATGInventoryComponent* InventoryComp = nullptr;

protected:
	// Id -> ItemWidget 맵(부분 갱신용)
	UPROPERTY(Transient)
	TMap<int32, TWeakObjectPtr<UATGInventoryItemWidget>> IdToWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UATGInventoryItemWidget> InventoryItemWidgetClass;

public:
	// 소유자 기준으로 인벤토리 찾아 자동 초기화(또는 BP에서 직접 Set 해도 됨)
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeFromOwner();


	// 전체 리빌드
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RebuildAll();
	void BindInventoryComp();
protected:
	virtual void NativeConstruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;


	// 델리게이트 핸들러
	UFUNCTION() void HandleItemAdded(int32 EntryId);
	UFUNCTION() void HandleItemChanged(int32 EntryId);
	UFUNCTION() void HandleItemRemoved(int32 EntryId);
	UFUNCTION() void HandleItemRotated(int32 EntryId);


	// 헬퍼
	const FInventoryEntry* FindEntryById(int32 EntryId) const;
	UATGInventoryItemWidget* CreateItemWidget(const FInventoryEntry& E);
	void UpdateItemSlot(UATGInventoryItemWidget* W, const FInventoryEntry& E);
	FIntPoint CellFromLocal(const FVector2D& Local) const;

	// 셀 배경 생성
	void BuildCellBackground();

protected:
	//드래그관련
	UDragDropOperation* Operation;
	bool bIsRotate = false;
};
