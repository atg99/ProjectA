// Fill out your copyright notice in the Description page of Project Settings.

#include "Widget/ATGEquipmentGridWidget.h"

#include "ATGDragDropOperation.h"
#include "ATGEquipmentComponent.h"
#include "ATGInventoryItemWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Data/ATGEquipmentData.h"
#include "Data/ATGItemData.h"

void UATGEquipmentGridWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UATGEquipmentGridWidget::BindInventoryComp()
{
	UATGEquipmentComponent* EquipmentComp = Cast<UATGEquipmentComponent>(Inven.GetObject());
	if (!EquipmentComp)
	{
		return;
	}

	EquipmentComp->OnFirstMainWeaponChanged.RemoveDynamic(this, &UATGEquipmentGridWidget::HandleEquipmentChanged);
	EquipmentComp->OnSecondMainWeaponChanged.RemoveDynamic(this, &UATGEquipmentGridWidget::HandleEquipmentChanged);

	switch (EquipmentSlot)
	{
	case EEquipmentSlotType::MainWeapon1Slot:
		EquipmentComp->OnFirstMainWeaponChanged.AddDynamic(this, &UATGEquipmentGridWidget::HandleEquipmentChanged);
		FitEquipmentType = EEquipmentType::Weapon;
		break;
	case EEquipmentSlotType::MainWeapon2Slot:
		EquipmentComp->OnSecondMainWeaponChanged.AddDynamic(this, &UATGEquipmentGridWidget::HandleEquipmentChanged);
		FitEquipmentType = EEquipmentType::Weapon;
		break;
	default:
		break;
	}

	RebuildAll();
}

void UATGEquipmentGridWidget::HandleEquipmentChanged(FInventoryEntry EquipmentEntry)
{
	if (!EquipmentEntry.Item)
	{
		if (EquipmentWidget)
		{
			EquipmentWidget->RemoveFromParent();
			EquipmentWidget = nullptr;
		}
		return;
	}

	if (!EquipmentWidget)
	{
		EquipmentWidget = CreateItemWidget(EquipmentEntry);
	}

	if (EquipmentWidget)
	{
		EquipmentWidget->RefreshFromEntry(EquipmentEntry, CellSize, CellPadding);
		UpdateItemSlot(EquipmentWidget, EquipmentEntry);
	}
}

void UATGEquipmentGridWidget::RebuildAll()
{
	if (!GridPanel || !Inven)
	{
		return;
	}

	GridPanel->ClearChildren();
	GridPanel->InvalidateLayoutAndVolatility();
	IdToWidget.Empty();
	BuildCellBackground();

	FInventoryEntry Equipment;
	Inven->GetEquipmentEntry(EquipmentSlot, Equipment);
	if (Equipment.Item)
	{
		EquipmentWidget = CreateItemWidget(Equipment);
		UpdateItemSlot(EquipmentWidget, Equipment);
	}
}

void UATGEquipmentGridWidget::BuildCellBackground()
{
	if (!GridPanel || !Inven)
	{
		return;
	}

	int32 Width = 0;
	int32 Height = 0;

	UATGEquipmentComponent* EquipmentComp = Cast<UATGEquipmentComponent>(Inven.GetObject());
	if (!EquipmentComp)
	{
		return;
	}

	switch (EquipmentSlot)
	{
	case EEquipmentSlotType::MainWeapon1Slot:
	case EEquipmentSlotType::MainWeapon2Slot:
		Width = EquipmentComp->WeaponSlotSize.X;
		Height = EquipmentComp->WeaponSlotSize.Y;
		break;
	default:
		return;
	}

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			USizeBox* CellBox = NewObject<USizeBox>(this);
			CellBox->SetWidthOverride(CellSize);
			CellBox->SetHeightOverride(CellSize);

			UImage* Cell = NewObject<UImage>(this);
			if (DefaultCellBg)
			{
				Cell->SetBrushFromTexture(DefaultCellBg);
			}
			else
			{
				Cell->SetColorAndOpacity(BGColor);
			}
			DefaultColor = Cell->GetColorAndOpacity();
			CellBox->AddChild(Cell);

			UGridSlot* CellSlot = GridPanel->AddChildToGrid(CellBox, Y, X);
			CellSlot->SetPadding(FMargin(CellPadding));
		}
	}
}

void UATGEquipmentGridWidget::HandleIncomingItem(UDragDropOperation* InOperation, UATGInventoryItemWidget* InDragged, FVector2D Screen)
{
	if (!InDragged || InDragged->ItemDef.IsNull())
	{
		return;
	}

	UATGItemData* ItemData = InDragged->ItemDef.Get();
	if (!ItemData)
	{
		ItemData = InDragged->ItemDef.LoadSynchronous();
	}

	if (CheckFitEquip(ItemData))
	{
		Inven->TryAddItemAt(InDragged->Inven, InDragged->EntryId, InDragged->ItemDef, InDragged->Quantity, static_cast<int32>(EquipmentSlot), 0);
	}
}

bool UATGEquipmentGridWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!Inven || !GridPanel)
	{
		return false;
	}

	UATGInventoryItemWidget* Dragged = InOperation ? Cast<UATGInventoryItemWidget>(InOperation->Payload) : nullptr;
	if (!Dragged)
	{
		return false;
	}

	const FVector2D Screen = InDragDropEvent.GetScreenSpacePosition();
	const FGeometry PanelGeo = GridPanel->GetTickSpaceGeometry();
	const FVector2D Local = PanelGeo.AbsoluteToLocal(Screen);

	FIntPoint Cell = CellFromLocal(Local);
	Cell.X = FMath::Clamp(Cell.X, 0, Inven->GetGridWidth() - 1);
	Cell.Y = FMath::Clamp(Cell.Y, 0, Inven->GetGridHeight() - 1);

	if (bIsDragLeave || PrevCell == Cell)
	{
		return false;
	}
	PrevCell = Cell;

	const FInventoryEntry* Entry = Dragged->Inven ? Dragged->Inven->GetInventory().GetById(Dragged->EntryId) : nullptr;
	if (!Entry)
	{
		return false;
	}

	UATGDragDropOperation* Op = Cast<UATGDragDropOperation>(InOperation);
	if (!Op)
	{
		return false;
	}

	int32 Width = 0;
	int32 Height = 0;
	if (Entry->bRotated)
	{
		Width = Op->bIsRotated ? Entry->Width : Entry->Height;
		Height = Op->bIsRotated ? Entry->Height : Entry->Width;
	}
	else
	{
		Width = Op->bIsRotated ? Entry->Height : Entry->Width;
		Height = Op->bIsRotated ? Entry->Width : Entry->Height;
	}

	bool bCanMove = Inven->CheckCanMove(Cell.X, Cell.Y, Width, Height, Entry->Id);
	UATGItemData* ItemData = Entry->Item.Get();
	if (!ItemData)
	{
		ItemData = Entry->Item.LoadSynchronous();
	}
	if (!CheckFitEquip(ItemData))
	{
		bCanMove = false;
	}

	for (UWidget* Child : GridPanel->GetAllChildren())
	{
		USizeBox* CellBox = Cast<USizeBox>(Child);
		UGridSlot* CellSlot = Child ? Cast<UGridSlot>(Child->Slot) : nullptr;
		if (!CellBox || !CellSlot)
		{
			continue;
		}

		bool bIsTargetGrid = false;
		for (int32 X = 0; X < Width && !bIsTargetGrid; ++X)
		{
			for (int32 Y = 0; Y < Height; ++Y)
			{
				if (CellSlot->GetColumn() == Cell.X + X && CellSlot->GetRow() == Cell.Y + Y)
				{
					bIsTargetGrid = true;
					break;
				}
			}
		}

		for (UWidget* CellChild : CellBox->GetAllChildren())
		{
			if (UImage* Img = Cast<UImage>(CellChild))
			{
				Img->SetColorAndOpacity(bIsTargetGrid ? (bCanMove ? CheckTrueColor : CheckFalseColor) : DefaultColor);
				CellSlot->SetLayer(bIsTargetGrid ? 1 : 0);
				break;
			}
		}
	}

	return false;
}

bool UATGEquipmentGridWidget::CheckIsFromOther(UATGInventoryItemWidget* Dragged)
{
	return Dragged && !bIsDragLeave;
}

bool UATGEquipmentGridWidget::CheckFitEquip(UATGItemData* ItemData)
{
	const UATGEquipmentData* EquipData = Cast<UATGEquipmentData>(ItemData);
	return EquipData && EquipData->EquipmentType == FitEquipmentType;
}
