// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGInventoryGirdWidget.h"

#include "ATGInventoryItemWidget.h"
#include "ATGInventoryComponent.h"

#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h" // 고정 셀 크기
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"

void UATGInventoryGirdWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(10, 3.0f, FColor::Red, TEXT("InvenGridWidget NativeConstruct"));

}

void UATGInventoryGirdWidget::InitializeFromOwner()
{
	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(10, 3.0f, FColor::Red, TEXT("InitializeFromOwner"));
	//if (APlayerController* PC = GetOwningPlayer())
	//{
	//	if (GEngine)
	//		GEngine->AddOnScreenDebugMessage(10, 3.0f, FColor::Red, TEXT("APlayerController"));
	//	if (APlayerState* PS = PC->GetPlayerState<APlayerState>())
	//	{
	//		if (GEngine)
	//			GEngine->AddOnScreenDebugMessage(10, 3.0f, FColor::Red, TEXT("APlayerState"));
	//		UATGInventoryComponent* Comp = PS->FindComponentByClass<UATGInventoryComponent>();
	//		if (Comp)
	//		{
	//			InventoryComp = Comp;
	//		}
	//		else
	//		{
	//			if (GEngine)
	//				GEngine->AddOnScreenDebugMessage(10, 3.0f, FColor::Red, TEXT("fail find UATGInventoryComponent"));
	//		}
	//	}
	//}
}

void UATGInventoryGirdWidget::RebuildAll()
{
	if (!GridPanel || !InventoryComp) return;


	GridPanel->ClearChildren();
	IdToWidget.Empty();


	BuildCellBackground();


	for (const FInventoryEntry& E : InventoryComp->GetEntries())
	{
		UATGInventoryItemWidget* W = CreateItemWidget(E);
		UpdateItemSlot(W, E);
		IdToWidget.Add(E.Id, W);
	}
}

void UATGInventoryGirdWidget::BindInventoryComp()
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(10, 3.0f, FColor::Magenta, TEXT("Widget BindComp"));
	if (InventoryComp)
	{
		// 중복 바인딩 방지 차원에서 먼저 제거 후 바인딩
		InventoryComp->OnItemAdded.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemAdded);
		InventoryComp->OnItemChanged.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemChanged);
		InventoryComp->OnItemRemoved.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemRemoved);


		InventoryComp->OnItemAdded.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemAdded);
		InventoryComp->OnItemChanged.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemChanged);
		InventoryComp->OnItemRemoved.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemRemoved);
	}
	RebuildAll();
}

void UATGInventoryGirdWidget::BuildCellBackground()
{
	if (!GridPanel || !InventoryComp) return;


	const int32 W = InventoryComp->GetGridWidth();
	const int32 H = InventoryComp->GetGridHeight();


	for (int32 y = 0; y < H; ++y)
	{
		for (int32 x = 0; x < W; ++x)
		{
			// SizeBox로 셀 크기를 고정해 클릭/드래그 좌표와 시각적 격자 간 오차 제거
			USizeBox* CellBox = NewObject<USizeBox>(this);
			CellBox->SetWidthOverride(CellSize);
			CellBox->SetHeightOverride(CellSize);


			UImage* Cell = NewObject<UImage>(this);
			if (DefaultCellBg)
			{
				Cell->SetBrushFromTexture(DefaultCellBg);
			}
			CellBox->AddChild(Cell);


			UGridSlot* CellSlot = GridPanel->AddChildToGrid(CellBox, y, x);
			CellSlot->SetPadding(FMargin(CellPadding));
		}
	}
}

UATGInventoryItemWidget* UATGInventoryGirdWidget::CreateItemWidget(const FInventoryEntry& E)
{
	// 디자이너에서 만든 위젯 BP를 서브클래스로 쓰고 싶다면, 팩토리 부분을 BP 클래스로 교체
	if (!InventoryItemWidgetClass)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("!!!InventoryGridWidget -> InventoryItemWidgetClass Is Not Setting"));
		return nullptr;
	}
	UATGInventoryItemWidget* W = CreateWidget<UATGInventoryItemWidget>(GetOwningPlayer(), InventoryItemWidgetClass);
	W->SetupFromEntry(E, InventoryComp, CellSize, CellPadding);
	return W;
}

void UATGInventoryGirdWidget::UpdateItemSlot(UATGInventoryItemWidget* W, const FInventoryEntry& E)
{
	if (!GridPanel || !W) return;


	if (!W->GetParent())
	{
		UGridSlot* CellSlot = GridPanel->AddChildToGrid(W, E.Y, E.X);
		CellSlot->SetRowSpan(E.Height);
		CellSlot->SetColumnSpan(E.Width);
		CellSlot->SetPadding(FMargin(CellPadding));
	}
	else if (UGridSlot* GS = Cast<UGridSlot>(W->Slot))
	{
		GS->SetRow(E.Y);
		GS->SetColumn(E.X);
		GS->SetRowSpan(E.Height);
		GS->SetColumnSpan(E.Width);
	}
}

const FInventoryEntry* UATGInventoryGirdWidget::FindEntryById(int32 EntryId) const
{
	if (!InventoryComp) return nullptr;
	for (const FInventoryEntry& E : InventoryComp->GetEntries())
	{
		if (E.Id == EntryId) return &E;
	}
	return nullptr;
}


FIntPoint UATGInventoryGirdWidget::CellFromLocal(const FVector2D& Local) const
{
	const float Pitch = float(CellSize + 2 * CellPadding);
	int32 X = FMath::Max(0, int32(FMath::FloorToFloat(Local.X / Pitch)));
	int32 Y = FMath::Max(0, int32(FMath::FloorToFloat(Local.Y / Pitch)));
	return FIntPoint(X, Y);
}


bool UATGInventoryGirdWidget::NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!InventoryComp || !GridPanel) return false;


	if (UATGInventoryItemWidget* Dragged = InOperation ? Cast<UATGInventoryItemWidget>(InOperation->Payload) : nullptr)
	{
		const FVector2D Local = InGeo.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
		FIntPoint Cell = CellFromLocal(Local);


		// 안전 클램프(서버도 판정하지만 UX용으로 선제 클램프)
		Cell.X = FMath::Clamp(Cell.X, 0, InventoryComp->GetGridWidth() - 1);
		Cell.Y = FMath::Clamp(Cell.Y, 0, InventoryComp->GetGridHeight() - 1);


		InventoryComp->ServerMoveOrSwap(Dragged->EntryId, Cell.X, Cell.Y);
		return true;
	}
	return false;
}


void UATGInventoryGirdWidget::NativeOnDragEnter(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	//Super::NativeOnDragEnter(InGeo, InDragDropEvent, InOperation);
	// TODO: 클라 미리보기(가능/불가 하이라이트) 구현 시 여기서 셀 강조 처리
}


void UATGInventoryGirdWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	//Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	// TODO: 하이라이트 해제
}


// ===== 델리게이트 핸들러 =====
void UATGInventoryGirdWidget::HandleItemAdded(int32 EntryId)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("OnHandleItemAdded"));
	if (const FInventoryEntry* E = FindEntryById(EntryId))
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("FindEntryById"));
		UATGInventoryItemWidget* W = CreateItemWidget(*E);
		if (!W)
		{
			return;
		}
		UpdateItemSlot(W, *E);
		IdToWidget.Add(EntryId, W);
	}
}


void UATGInventoryGirdWidget::HandleItemChanged(int32 EntryId)
{
	UATGInventoryItemWidget* W = IdToWidget.FindRef(EntryId).Get();
	const FInventoryEntry* E = FindEntryById(EntryId);
	if (!E)
	{
		// 서버에서 제거되었는데 아직 위젯이 남아있을 수 있음
		if (W) { W->RemoveFromParent(); }
		IdToWidget.Remove(EntryId);
		return;
	}


	if (!W)
	{
		W = CreateItemWidget(*E);
		IdToWidget.Add(EntryId, W);
	}
	W->RefreshFromEntry(*E, CellSize, CellPadding);
	UpdateItemSlot(W, *E);
}


void UATGInventoryGirdWidget::HandleItemRemoved(int32 EntryId)
{
	if (UATGInventoryItemWidget* W = IdToWidget.FindRef(EntryId).Get())
	{
		W->RemoveFromParent();
	}
	IdToWidget.Remove(EntryId);
}