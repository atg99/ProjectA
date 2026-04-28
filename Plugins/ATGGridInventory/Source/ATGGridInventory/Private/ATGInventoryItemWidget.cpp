// Fill out your copyright notice in the Description page of Project Settings.

#include "ATGInventoryItemWidget.h"

#include "ATGDragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Data/ATGItemData.h"

void UATGInventoryItemWidget::SetupFromEntry(const TScriptInterface<IATGInventoryOwnerInterface> InInven, const FInventoryEntry& InEntry, int32 InCellSize, int32 InCellPadding)
{
	EntryId = InEntry.Id;
	DBId = InEntry.DBId;
	Inven = InInven;
	CachedEntry = InEntry;
	CachedCellSize = InCellSize;
	CachedCellPadding = InCellPadding;
	Quantity = InEntry.Quantity;
	bIsRotated = InEntry.bRotated;
	ItemDef = InEntry.Item;
	ItemData = InEntry.Item.Get();
	if (!ItemData)
	{
		ItemData = InEntry.Item.LoadSynchronous();
	}

	if (ItemIcon && ItemData && ItemData->Icon)
	{
		ItemIcon->SetBrushFromTexture(ItemData->Icon);
	}

	const float Pitch = float(InCellSize + 2 * InCellPadding);
	const float WidthPx = InEntry.Width * Pitch - 2 * InCellPadding;
	const float HeightPx = InEntry.Height * Pitch - 2 * InCellPadding;

	if (RootSizeBox)
	{
		RootSizeBox->SetWidthOverride(WidthPx);
		RootSizeBox->SetHeightOverride(HeightPx);
	}
}

void UATGInventoryItemWidget::RefreshFromEntry(const FInventoryEntry& InEntry, int32 InCellSize, int32 InCellPadding)
{
	EntryId = InEntry.Id;
	DBId = InEntry.DBId;
	CachedEntry = InEntry;
	CachedCellSize = InCellSize;
	CachedCellPadding = InCellPadding;
	Quantity = InEntry.Quantity;
	bIsRotated = InEntry.bRotated;
	ItemDef = InEntry.Item;
	ItemData = InEntry.Item.Get();
	if (!ItemData)
	{
		ItemData = InEntry.Item.LoadSynchronous();
	}

	const float Pitch = float(InCellSize + 2 * InCellPadding);
	const float WidthPx = InEntry.Width * Pitch - 2 * InCellPadding;
	const float HeightPx = InEntry.Height * Pitch - 2 * InCellPadding;

	if (RootSizeBox)
	{
		RootSizeBox->SetWidthOverride(WidthPx);
		RootSizeBox->SetHeightOverride(HeightPx);
	}
}

void UATGInventoryItemWidget::SetLockItem(bool bInIsLock)
{
	bIsLock = bInIsLock;
	if (!ItemIcon)
	{
		return;
	}

	ItemIcon->SetBrushTintColor(bIsLock ? LockColor : FLinearColor(1.f, 1.f, 1.f, 1.f));
}

FReply UATGInventoryItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeo, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		FATGItemInfo Info;
		Info.ItemWidget = this;
		Info.ItemDef = ItemDef;
		Info.Quantity = Quantity;
		Info.bIsRotated = bIsRotated;
		Info.EntryId = EntryId;

		FVector2D MousePosition = FVector2D::ZeroVector;
		if (APlayerController* PC = GetOwningPlayer())
		{
			double PosX = 0.0;
			double PosY = 0.0;
			PC->GetMousePosition(PosX, PosY);
			MousePosition = FVector2D(PosX, PosY);
		}

		if (Inven)
		{
			Inven->OpenContextMenu(Info, MousePosition);
		}
		return FReply::Handled();
	}

	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		if (OnItemPressed.IsBound())
		{
			FATGItemInfo Info;
			Info.ItemWidget = this;
			Info.ItemDef = ItemDef;
			Info.Quantity = Quantity;
			Info.bIsRotated = bIsRotated;
			Info.EntryId = EntryId;
			OnItemPressed.Broadcast(Info);
		}

		if (bCanDrag)
		{
			return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
		}
	}

	return Super::NativeOnMouseButtonDown(InGeo, InMouseEvent);
}

void UATGInventoryItemWidget::NativeOnDragDetected(const FGeometry& InGeo, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	UATGDragDropOperation* Op = NewObject<UATGDragDropOperation>(this);

	UATGInventoryItemWidget* Visual = CreateWidget<UATGInventoryItemWidget>(GetOwningPlayer(), GetClass());
	if (Visual)
	{
		Visual->SetupFromEntry(Inven, CachedEntry, CachedCellSize, CachedCellPadding);
		Visual->SetVisibility(ESlateVisibility::HitTestInvisible);
		Visual->SetRenderOpacity(0.4f);
		Op->DefaultDragVisual = Visual;
		Op->Pivot = EDragPivot::TopLeft;
		Op->bIsRotated = bIsRotated;
	}

	Op->Payload = this;
	OutOperation = Op;
}
