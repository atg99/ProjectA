// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGInventoryGirdWidget.h"

#include "ATGInventoryItemWidget.h"
#include "ATGInventoryComponent.h"

#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h" // ���� �� ũ��
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
		// �ߺ� ���ε� ���� �������� ���� ���� �� ���ε�
		InventoryComp->OnItemAdded.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemAdded);
		InventoryComp->OnItemChanged.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemChanged);
		InventoryComp->OnItemRemoved.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemRemoved);
		InventoryComp->OnItemRotated.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemRotated);
		//preview
		InventoryComp->OnItemPreAdded.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemPreAdded);
		InventoryComp->OnItemPreRemoved.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemPreRemoved);

		InventoryComp->OnItemAdded.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemAdded);
		InventoryComp->OnItemChanged.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemChanged);
		InventoryComp->OnItemRemoved.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemRemoved);
		InventoryComp->OnItemRotated.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemRotated);
		//preview
		InventoryComp->OnItemPreAdded.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemPreAdded);
		InventoryComp->OnItemPreRemoved.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemPreRemoved);
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
			// SizeBox�� �� ũ�⸦ ������ Ŭ��/�巡�� ��ǥ�� �ð��� ���� �� ���� ����
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
			CellBox->AddChild(Cell);


			UGridSlot* CellSlot = GridPanel->AddChildToGrid(CellBox, y, x);
			CellSlot->SetPadding(FMargin(CellPadding));
		}
	}
}

UATGInventoryItemWidget* UATGInventoryGirdWidget::CreateItemWidget(const FInventoryEntry& E)
{
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
		const FVector2D Screen = InDragDropEvent.GetScreenSpacePosition();
		const FGeometry PanelGeo = GridPanel->GetTickSpaceGeometry();
		const FVector2D Local = PanelGeo.AbsoluteToLocal(Screen);
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("Local")+Local.ToString());
		FIntPoint Cell = CellFromLocal(Local);
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("Cell")+Cell.ToString());


		// ���� Ŭ����(������ ���������� UX������ ���� Ŭ����)
		Cell.X = FMath::Clamp(Cell.X, 0, InventoryComp->GetGridWidth() - 1);
		Cell.Y = FMath::Clamp(Cell.Y, 0, InventoryComp->GetGridHeight() - 1);


		InventoryComp->ServerMoveOrSwap(Dragged->EntryId, Cell.X, Cell.Y, bIsRotate);

		Operation = nullptr;
		bIsRotate = false;

		return true;
	}

	Operation = nullptr;
	bIsRotate = false;

	return false;
}


void UATGInventoryGirdWidget::NativeOnDragEnter(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	//Super::NativeOnDragEnter(InGeo, InDragDropEvent, InOperation);
	// TODO: Ŭ�� �̸�����(����/�Ұ� ���̶���Ʈ) ���� �� ���⼭ �� ���� ó��
	Operation = InOperation;
}
 

void UATGInventoryGirdWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	//Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	// TODO: ���̶���Ʈ ����
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("NativeOnDragLeave"));
	Operation = nullptr;
	bIsRotate = false;
}


// ===== ��������Ʈ �ڵ鷯 =====
void UATGInventoryGirdWidget::HandleItemAdded(int32 EntryId)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("OnHandleItemAdded"));
	HandleItemPreRemoved(EntryId);

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
		// �������� ���ŵǾ��µ� ���� ������ �������� �� ����
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

void UATGInventoryGirdWidget::HandleItemRotated(int32 EntryId)
{
	if (UWidget* Ghost = Operation ? Operation->DefaultDragVisual : nullptr)
	{
		//90�� �ð� ȸ��
		Ghost->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		FWidgetTransform T = Ghost->GetRenderTransform();
		T.Angle += 90.f;
		Ghost->SetRenderTransform(T);

		bIsRotate = !bIsRotate;
	}
}

void UATGInventoryGirdWidget::HandleItemPreAdded(FInventoryEntry PreE)
{

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("HandleItemPreAdded"));
	UATGInventoryItemWidget* W = CreateItemWidget(PreE);
	if (!W)
	{
		return;
	}
	W->SetVisibility(ESlateVisibility::HitTestInvisible); //preview widget ��ȣ�ۿ� �Ұ�
	W->ItemIcon->SetColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 0.5f));
	UpdateItemSlot(W, PreE);
	PreviewIdToWidget.Add(PreE.Id, W);
}

void UATGInventoryGirdWidget::HandleItemPreRemoved(int32 PreEId)
{
	if (UATGInventoryItemWidget* W = PreviewIdToWidget.FindRef(PreEId).Get())
	{
		W->RemoveFromParent();
	}
	PreviewIdToWidget.Remove(PreEId);
}
