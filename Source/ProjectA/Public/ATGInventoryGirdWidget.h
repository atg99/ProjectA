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
	// �����̳ʿ��� GridPanel ���ε� (Named Slot: GridPanel)
	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	UGridPanel* GridPanel = nullptr;

	// �ȼ� ���� �� ũ�� & �е�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout")
	int32 CellSize = 64;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Layout")
	int32 CellPadding = 1;


	// �� ��� �ؽ�ó(�ɼ�)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Skin")
	UTexture2D* DefaultCellBg = nullptr;


	// �κ��丮 �ҽ�
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	UATGInventoryComponent* InventoryComp = nullptr;

protected:
	// Id -> ItemWidget ��(�κ� ���ſ�)
	UPROPERTY(Transient)
	TMap<int32, TWeakObjectPtr<UATGInventoryItemWidget>> IdToWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UATGInventoryItemWidget> InventoryItemWidgetClass;

public:
	// ������ �������� �κ��丮 ã�� �ڵ� �ʱ�ȭ(�Ǵ� BP���� ���� Set �ص� ��)
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeFromOwner();


	// ��ü ������
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RebuildAll();
	void BindInventoryComp();
protected:
	virtual void NativeConstruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;


	// ��������Ʈ �ڵ鷯
	UFUNCTION() void HandleItemAdded(int32 EntryId);
	UFUNCTION() void HandleItemChanged(int32 EntryId);
	UFUNCTION() void HandleItemRemoved(int32 EntryId);
	UFUNCTION() void HandleItemRotated(int32 EntryId);


	// ����
	const FInventoryEntry* FindEntryById(int32 EntryId) const;
	UATGInventoryItemWidget* CreateItemWidget(const FInventoryEntry& E);
	void UpdateItemSlot(UATGInventoryItemWidget* W, const FInventoryEntry& E);
	FIntPoint CellFromLocal(const FVector2D& Local) const;

	// �� ��� ����
	void BuildCellBackground();

protected:
	//�巡�װ���
	UDragDropOperation* Operation;
	bool bIsRotate = false;
};
