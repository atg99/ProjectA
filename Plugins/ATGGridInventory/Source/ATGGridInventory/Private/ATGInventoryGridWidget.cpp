// Fill out your copyright notice in the Description page of Project Settings.

#include "ATGInventoryGridWidget.h"

#include "ATGContainerComponent.h"
#include "ATGDragDropOperation.h"
#include "ATGInventoryComponent.h"
#include "ATGInventoryItemWidget.h"
#include "ATGStackSplitWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/Button.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Data/ATGItemData.h"

void UATGInventoryGridWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Sort)
	{
		Btn_Sort->OnClicked.AddDynamic(this, &UATGInventoryGridWidget::OnSortBtnClicked);
	}
}

void UATGInventoryGridWidget::InitializeFromOwner()
{
}

void UATGInventoryGridWidget::RebuildAll()
{
	if (!GridPanel || !Inven)
	{
		return;
	}

	GridPanel->ClearChildren();
	GridPanel->InvalidateLayoutAndVolatility();
	IdToWidget.Empty();

	BuildCellBackground();

	for (const FInventoryEntry& Entry : Inven->GetEntries())
	{
		if (UATGInventoryItemWidget* Widget = CreateItemWidget(Entry))
		{
			UpdateItemSlot(Widget, Entry);
			IdToWidget.Add(Entry.Id, Widget);
		}
	}

	OnGridRebuild.Broadcast(IdToWidget.Num());
}

void UATGInventoryGridWidget::BindInventoryComp()
{
	if (UATGInventoryComponent* InventoryComp = Cast<UATGInventoryComponent>(Inven.GetObject()))
	{
		InventoryComp->OnItemAdded.RemoveDynamic(this, &UATGInventoryGridWidget::HandleItemAdded);
		InventoryComp->OnItemChanged.RemoveDynamic(this, &UATGInventoryGridWidget::HandleItemChanged);
		InventoryComp->OnItemRemoved.RemoveDynamic(this, &UATGInventoryGridWidget::HandleItemRemoved);
		InventoryComp->OnRebuildAll.RemoveDynamic(this, &UATGInventoryGridWidget::HandleRebuildAll);
		InventoryComp->OnItemPreAdded.RemoveDynamic(this, &UATGInventoryGridWidget::HandleItemPreAdded);
		InventoryComp->OnItemPreChanged.RemoveDynamic(this, &UATGInventoryGridWidget::HandleItemPreChanged);
		InventoryComp->OnItemPreRemoved.RemoveDynamic(this, &UATGInventoryGridWidget::HandleItemPreRemoved);

		InventoryComp->OnItemAdded.AddDynamic(this, &UATGInventoryGridWidget::HandleItemAdded);
		InventoryComp->OnItemChanged.AddDynamic(this, &UATGInventoryGridWidget::HandleItemChanged);
		InventoryComp->OnItemRemoved.AddDynamic(this, &UATGInventoryGridWidget::HandleItemRemoved);
		InventoryComp->OnRebuildAll.AddDynamic(this, &UATGInventoryGridWidget::HandleRebuildAll);
	}
	else if (UATGContainerComponent* ContainerComp = Cast<UATGContainerComponent>(Inven.GetObject()))
	{
		ContainerComp->OnContainerAdded.RemoveDynamic(this, &UATGInventoryGridWidget::HandleItemAdded);
		ContainerComp->OnContainerChanged.RemoveDynamic(this, &UATGInventoryGridWidget::HandleItemChanged);
		ContainerComp->OnContainerRemoved.RemoveDynamic(this, &UATGInventoryGridWidget::HandleItemRemoved);
		ContainerComp->OnRebuildAll.RemoveDynamic(this, &UATGInventoryGridWidget::HandleRebuildAll);

		ContainerComp->OnContainerAdded.AddDynamic(this, &UATGInventoryGridWidget::HandleItemAdded);
		ContainerComp->OnContainerChanged.AddDynamic(this, &UATGInventoryGridWidget::HandleItemChanged);
		ContainerComp->OnContainerRemoved.AddDynamic(this, &UATGInventoryGridWidget::HandleItemRemoved);
		ContainerComp->OnRebuildAll.AddDynamic(this, &UATGInventoryGridWidget::HandleRebuildAll);
	}

	RebuildAll();
}

int32 UATGInventoryGridWidget::GetDBIdByEntryId(int32 ItemEntryId)
{
	const FInventoryEntry* Entry = FindEntryById(ItemEntryId);
	return Entry ? Entry->DBId : -1;
}

void UATGInventoryGridWidget::BuildCellBackground()
{
	if (!GridPanel || !Inven)
	{
		return;
	}

	for (int32 Y = 0; Y < Inven->GetGridHeight(); ++Y)
	{
		for (int32 X = 0; X < Inven->GetGridWidth(); ++X)
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

void UATGInventoryGridWidget::HandleIncomingItem(UDragDropOperation* InOperation, UATGInventoryItemWidget* InDragged, FVector2D Screen)
{
	if (!InDragged || !Inven)
	{
		return;
	}

	const FGeometry PanelGeo = GridPanel->GetTickSpaceGeometry();
	const FVector2D Local = PanelGeo.AbsoluteToLocal(Screen);
	FIntPoint Cell = CellFromLocal(Local);
	Cell.X = FMath::Clamp(Cell.X, 0, Inven->GetGridWidth() - 1);
	Cell.Y = FMath::Clamp(Cell.Y, 0, Inven->GetGridHeight() - 1);

	bool bIsRotated = false;
	if (UATGDragDropOperation* Op = Cast<UATGDragDropOperation>(InOperation))
	{
		bIsRotated = Op->bIsRotated;
	}

	Inven->TryAddItemAt(InDragged->Inven, InDragged->EntryId, InDragged->ItemDef, InDragged->Quantity, Cell.X, Cell.Y, bIsRotated);
}

UATGInventoryItemWidget* UATGInventoryGridWidget::CreateItemWidget(const FInventoryEntry& E)
{
	if (!InventoryItemWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("UATGInventoryGridWidget::CreateItemWidget InventoryItemWidgetClass is not set"));
		return nullptr;
	}

	UATGInventoryItemWidget* Widget = CreateWidget<UATGInventoryItemWidget>(GetOwningPlayer(), InventoryItemWidgetClass);
	if (Widget)
	{
		Widget->SetupFromEntry(Inven, E, CellSize, CellPadding);
		Widget->bCanDrag = bCanDrag;
	}
	return Widget;
}

void UATGInventoryGridWidget::UpdateItemSlot(UATGInventoryItemWidget* W, const FInventoryEntry& E)
{
	if (!GridPanel || !W)
	{
		return;
	}

	if (!W->GetParent())
	{
		UGridSlot* CellSlot = GridPanel->AddChildToGrid(W, E.Y, E.X);
		CellSlot->SetRowSpan(E.Height);
		CellSlot->SetColumnSpan(E.Width);
		CellSlot->SetPadding(FMargin(CellPadding));
	}
	else if (UGridSlot* GridSlot = Cast<UGridSlot>(W->Slot))
	{
		GridSlot->SetRow(E.Y);
		GridSlot->SetColumn(E.X);
		GridSlot->SetRowSpan(E.Height);
		GridSlot->SetColumnSpan(E.Width);
	}
}

const FInventoryEntry* UATGInventoryGridWidget::FindEntryById(int32 EntryId) const
{
	if (!Inven)
	{
		return nullptr;
	}

	for (const FInventoryEntry& Entry : Inven->GetEntries())
	{
		if (Entry.Id == EntryId)
		{
			return &Entry;
		}
	}
	return nullptr;
}

FIntPoint UATGInventoryGridWidget::CellFromLocal(const FVector2D& Local) const
{
	const float Pitch = float(CellSize + 2 * CellPadding);
	const int32 X = FMath::Max(0, int32(FMath::FloorToFloat(Local.X / Pitch)));
	const int32 Y = FMath::Max(0, int32(FMath::FloorToFloat(Local.Y / Pitch)));
	return FIntPoint(X, Y);
}

bool UATGInventoryGridWidget::CheckIsOutGrid(const FVector2D& Local) const
{
	if (!Inven || Local.X < 0.f || Local.Y < 0.f)
	{
		return true;
	}

	const float Pitch = float(CellSize + 2 * CellPadding);
	return Local.X > Pitch * Inven->GetGridWidth() || Local.Y > Pitch * Inven->GetGridHeight();
}

void UATGInventoryGridWidget::DoNativeOnDrop(UDragDropOperation* InOperation, UATGInventoryItemWidget* Dragged, FVector2D Screen)
{
	const FGeometry PanelGeo = GridPanel->GetTickSpaceGeometry();
	const FVector2D Local = PanelGeo.AbsoluteToLocal(Screen);

	if (bIsDragLeave)
	{
		Inven->TryDropItem(Dragged->EntryId);
		return;
	}

	FIntPoint Cell = CellFromLocal(Local);
	Cell.X = FMath::Clamp(Cell.X, 0, Inven->GetGridWidth() - 1);
	Cell.Y = FMath::Clamp(Cell.Y, 0, Inven->GetGridHeight() - 1);

	bool bIsRotated = false;
	if (UATGDragDropOperation* Op = Cast<UATGDragDropOperation>(InOperation))
	{
		bIsRotated = Op->bIsRotated;
	}

	bool bShouldRotate = false;
	if (const FInventoryEntry* Entry = Inven->GetInventory().GetById(Dragged->EntryId))
	{
		bShouldRotate = Entry->bRotated ? !bIsRotated : bIsRotated;
	}

	Inven->TryMoveOrSwapClient(Dragged->EntryId, Cell.X, Cell.Y, bShouldRotate);
}

void UATGInventoryGridWidget::DoNativeOnDrop(UDragDropOperation* InOperation, UATGInventoryItemWidget* Dragged, FVector2D Screen, int32 SplitNum)
{
	const FGeometry PanelGeo = GridPanel->GetTickSpaceGeometry();
	const FVector2D Local = PanelGeo.AbsoluteToLocal(Screen);

	if (CheckIsOutGrid(Local))
	{
		Inven->TryDropItem(Dragged->EntryId, SplitNum);
		return;
	}

	FIntPoint Cell = CellFromLocal(Local);
	Cell.X = FMath::Clamp(Cell.X, 0, Inven->GetGridWidth() - 1);
	Cell.Y = FMath::Clamp(Cell.Y, 0, Inven->GetGridHeight() - 1);

	bool bIsRotated = false;
	if (UATGDragDropOperation* Op = Cast<UATGDragDropOperation>(InOperation))
	{
		bIsRotated = Op->bIsRotated;
	}

	bool bShouldRotate = false;
	if (const FInventoryEntry* Entry = Inven->GetInventory().GetById(Dragged->EntryId))
	{
		bShouldRotate = Entry->bRotated ? !bIsRotated : bIsRotated;
	}

	Inven->TrySplitStack(Dragged->EntryId, Cell.X, Cell.Y, bShouldRotate, SplitNum);
}

bool UATGInventoryGridWidget::NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!bCanDrag)
	{
		return false;
	}

	ON_SCOPE_EXIT
	{
		SetAllGridDefault();
	};

	if (!Inven || !GridPanel)
	{
		return true;
	}

	UATGInventoryItemWidget* Dragged = InOperation ? Cast<UATGInventoryItemWidget>(InOperation->Payload) : nullptr;
	if (!Dragged)
	{
		return true;
	}

	const FVector2D Screen = InDragDropEvent.GetScreenSpacePosition();
	if (CheckIsFromOther(Dragged))
	{
		HandleIncomingItem(InOperation, Dragged, Screen);
		return true;
	}

	if (Dragged->Inven != Inven)
	{
		return true;
	}

	if (InDragDropEvent.IsControlDown() && Dragged->Quantity != 1)
	{
		if (!SplitUI && StackSplitWidgetClass)
		{
			SplitUI = CreateWidget<UATGStackSplitWidget>(GetWorld(), StackSplitWidgetClass);
			if (SplitUI)
			{
				SplitUI->AddToViewport();
			}
		}

		if (SplitUI)
		{
			SplitUI->OnSplitConfirmed.Clear();
			SplitUI->OnSplitConfirmed.AddLambda([this, Dragged, Screen, InOperation](int32 SplitNum)
			{
				DoNativeOnDrop(InOperation, Dragged, Screen, SplitNum);
			});
			SplitUI->SetVisibility(ESlateVisibility::Visible);
			SplitUI->InitSplit(Dragged->Quantity);
		}
		return true;
	}

	DoNativeOnDrop(InOperation, Dragged, Screen);
	return true;
}

bool UATGInventoryGridWidget::CheckIsFromOther(UATGInventoryItemWidget* Dragged)
{
	return Dragged && Dragged->Inven != Inven && !bIsDragLeave;
}

void UATGInventoryGridWidget::NativeOnDragEnter(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	bIsDragLeave = false;
}

void UATGInventoryGridWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	SetAllGridDefault();
}

bool UATGInventoryGridWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
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

	UATGDragDropOperation* Op = Cast<UATGDragDropOperation>(InOperation);
	if (!Op)
	{
		return false;
	}

	int32 Width = 0;
	int32 Height = 0;
	int32 IgnoreId = -1;

	const FInventoryEntry* Entry = Inven->GetInventory().GetById(Dragged->EntryId);
	if (!Entry)
	{
		if (!CheckIsFromOther(Dragged))
		{
			return false;
		}

		UATGItemData* ItemData = Dragged->ItemDef.Get();
		if (!ItemData)
		{
			ItemData = Dragged->ItemDef.LoadSynchronous();
		}
		if (!ItemData)
		{
			return false;
		}

		Width = Op->bIsRotated ? ItemData->Height : ItemData->Width;
		Height = Op->bIsRotated ? ItemData->Width : ItemData->Height;
	}
	else
	{
		IgnoreId = Entry->Id;
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
	}

	const bool bCanMove = Inven->CheckCanMove(Cell.X, Cell.Y, Width, Height, IgnoreId);

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

void UATGInventoryGridWidget::SetAllGridDefault()
{
	bIsDragLeave = true;
	PrevCell = FIntPoint(-1, -1);

	if (!GridPanel)
	{
		return;
	}

	for (UWidget* Child : GridPanel->GetAllChildren())
	{
		USizeBox* CellBox = Cast<USizeBox>(Child);
		if (!CellBox)
		{
			continue;
		}

		for (UWidget* CellChild : CellBox->GetAllChildren())
		{
			if (UImage* Img = Cast<UImage>(CellChild))
			{
				Img->SetColorAndOpacity(DefaultColor);
				if (UGridSlot* CellSlot = Cast<UGridSlot>(Child->Slot))
				{
					CellSlot->SetLayer(0);
				}
				break;
			}
		}
	}
}

void UATGInventoryGridWidget::HandleItemAdded(int32 EntryId)
{
	if (const FInventoryEntry* Entry = FindEntryById(EntryId))
	{
		if (UATGInventoryItemWidget* Widget = CreateItemWidget(*Entry))
		{
			UpdateItemSlot(Widget, *Entry);
			IdToWidget.Add(EntryId, Widget);
		}
	}
}

void UATGInventoryGridWidget::HandleItemChanged(int32 EntryId)
{
	HandleItemPreRemoved(EntryId);

	UATGInventoryItemWidget* Widget = IdToWidget.FindRef(EntryId).Get();
	const FInventoryEntry* Entry = FindEntryById(EntryId);
	if (!Entry)
	{
		if (Widget)
		{
			Widget->RemoveFromParent();
		}
		IdToWidget.Remove(EntryId);
		return;
	}

	if (!Widget)
	{
		Widget = CreateItemWidget(*Entry);
		IdToWidget.Add(EntryId, Widget);
	}

	if (Widget)
	{
		Widget->RefreshFromEntry(*Entry, CellSize, CellPadding);
		UpdateItemSlot(Widget, *Entry);
	}
}

void UATGInventoryGridWidget::HandleItemRemoved(int32 EntryId)
{
	if (UATGInventoryItemWidget* Widget = IdToWidget.FindRef(EntryId).Get())
	{
		Widget->RemoveFromParent();
	}
	IdToWidget.Remove(EntryId);
}

void UATGInventoryGridWidget::HandleRebuildAll(int32 EntryId)
{
	RebuildAll();
}

void UATGInventoryGridWidget::HandleItemPreAdded(FInventoryEntry PreE)
{
	UATGInventoryItemWidget* Widget = CreateItemWidget(PreE);
	if (!Widget)
	{
		return;
	}

	Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (Widget->ItemIcon)
	{
		Widget->ItemIcon->SetColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 0.5f));
	}
	UpdateItemSlot(Widget, PreE);
	PreviewIdToWidget.FindOrAdd(PreE.Id).Add(Widget);
}

void UATGInventoryGridWidget::HandleItemPreChanged(FInventoryEntry PreE)
{
	if (TArray<TWeakObjectPtr<UATGInventoryItemWidget>>* Widgets = PreviewIdToWidget.Find(PreE.Id))
	{
		for (TWeakObjectPtr<UATGInventoryItemWidget>& WeakWidget : *Widgets)
		{
			if (UATGInventoryItemWidget* Widget = WeakWidget.Get())
			{
				Widget->RefreshFromEntry(PreE, CellSize, CellPadding);
			}
		}
	}
}

void UATGInventoryGridWidget::HandleItemPreRemoved(int32 PreEId)
{
	if (TArray<TWeakObjectPtr<UATGInventoryItemWidget>>* Widgets = PreviewIdToWidget.Find(PreEId))
	{
		for (TWeakObjectPtr<UATGInventoryItemWidget>& WeakWidget : *Widgets)
		{
			if (UATGInventoryItemWidget* Widget = WeakWidget.Get())
			{
				Widget->RemoveFromParent();
			}
		}
		PreviewIdToWidget.Remove(PreEId);
	}
}

void UATGInventoryGridWidget::OnSortBtnClicked()
{
	if (Inven)
	{
		Inven->TrySortByItemId();
	}
}
