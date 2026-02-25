// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGInventoryGirdWidget.h"

#include "ATGInventoryItemWidget.h"
#include "ATGInventoryComponent.h"
#include "InventoryTypes.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h" // 고정 셀 크기

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "ATGStackSplitWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "ATGPlayerController.h"
#include "ATGContainerComponent.h"
#include "ATGDragDropOperation.h"

#include "Utils/NetworkUtil.h"
#include "Data/ATGItemData.h"


void UATGInventoryGirdWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(10, 3.0f, FColor::Red, TEXT("InvenGridWidget NativeConstruct"));

	if (Btn_Sort)
	{
		UE_LOG(LogTemp, Display, TEXT("Valid Btn_Sort"));

		Btn_Sort->OnClicked.AddDynamic(this, &UATGInventoryGirdWidget::OnSortBtnClicked);
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("Invalid Btn_Sort"));
	}
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
	if (!GridPanel || !Inven) return;

	GridPanel->ClearChildren();
	GridPanel->InvalidateLayoutAndVolatility(); // 레이아웃 새로 계산 강제

	IdToWidget.Empty();

	BuildCellBackground();

	for (const FInventoryEntry& E : Inven->GetEntries())
	{
		UATGInventoryItemWidget* W = CreateItemWidget(E);
		UpdateItemSlot(W, E);
		IdToWidget.Add(E.Id, W);
	}
	
	if (OnGridRebuild.IsBound())
	{
		OnGridRebuild.Broadcast(IdToWidget.Num());
	}
}

void UATGInventoryGirdWidget::BindInventoryComp()
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(10, 3.0f, FColor::Magenta, TEXT("Widget BindComp"));
	if (UATGInventoryComponent* InventoryComp = Cast<UATGInventoryComponent>(Inven.GetObject()))
	{
		// 중복 바인딩 방지 차원에서 먼저 제거 후 바인딩
		InventoryComp->OnItemAdded.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemAdded);
		InventoryComp->OnItemChanged.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemChanged);
		InventoryComp->OnItemRemoved.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemRemoved);
		InventoryComp->OnRebuildAll.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleRebuildAll);
		//InventoryComp->OnItemRotated.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemRotated);

		//preview
		InventoryComp->OnItemPreAdded.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemPreAdded);
		InventoryComp->OnItemPreChanged.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemPreChanged);
		InventoryComp->OnItemPreRemoved.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemPreRemoved);

		InventoryComp->OnItemAdded.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemAdded);
		InventoryComp->OnItemChanged.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemChanged);
		InventoryComp->OnItemRemoved.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemRemoved);
		InventoryComp->OnRebuildAll.AddDynamic(this, &UATGInventoryGirdWidget::HandleRebuildAll);
		//InventoryComp->OnItemRotated.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemRotated);

		//preview
		//InventoryComp->OnItemPreAdded.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemPreAdded);
		//InventoryComp->OnItemPreChanged.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemPreChanged);
		//InventoryComp->OnItemPreRemoved.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemPreRemoved);
	}
	else if (UATGContainerComponent* ContainerComp = Cast<UATGContainerComponent>(Inven.GetObject()))
	{
		ContainerComp->OnContainerAdded.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemAdded);
		ContainerComp->OnContainerChanged.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemChanged);
		ContainerComp->OnContainerRemoved.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemRemoved);
		ContainerComp->OnRebuildAll.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleRebuildAll);
		//ContainerComp->OnContainerRotated.RemoveDynamic(this, &UATGInventoryGirdWidget::HandleItemRotated);

		ContainerComp->OnContainerAdded.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemAdded);
		ContainerComp->OnContainerChanged.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemChanged);
		ContainerComp->OnContainerRemoved.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemRemoved);
		ContainerComp->OnRebuildAll.AddDynamic(this, &UATGInventoryGirdWidget::HandleRebuildAll);
		//ContainerComp->OnContainerRotated.AddDynamic(this, &UATGInventoryGirdWidget::HandleItemRotated);
	}

	RebuildAll();
}

int32 UATGInventoryGirdWidget::GetDBIdByEntryId(int32 ItemEntryId)
{
	const FInventoryEntry* Entry = FindEntryById(ItemEntryId);
	if (Entry)
	{
		return Entry->DBId;
	}

	return -1;
}

void UATGInventoryGirdWidget::BuildCellBackground()
{
	if (!GridPanel || !Inven) return;

	const int32 W = Inven->GetGridWidth();
	const int32 H = Inven->GetGridHeight();

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
			else
			{
				Cell->SetColorAndOpacity(BGColor);
			}
			DefaultColor = Cell->GetColorAndOpacity();
			CellBox->AddChild(Cell);

			//GridPanel->GetSlots()
			UGridSlot* CellSlot = GridPanel->AddChildToGrid(CellBox, y, x);
			CellSlot->SetPadding(FMargin(CellPadding));
			
		}
	}
}

void UATGInventoryGirdWidget::HandleIncomingItem(UDragDropOperation* InOperation, UATGInventoryItemWidget* InDragged, FVector2D Screen)
{
	const FGeometry PanelGeo = GridPanel->GetTickSpaceGeometry();

	const FVector2D Local = PanelGeo.AbsoluteToLocal(Screen);

	FIntPoint Cell = CellFromLocal(Local);

	Cell.X = FMath::Clamp(Cell.X, 0, Inven->GetGridWidth() - 1);
	Cell.Y = FMath::Clamp(Cell.Y, 0, Inven->GetGridHeight() - 1);
	//InDragged->Inven;

	bool bIsR = false;
	UATGDragDropOperation* Op = Cast<UATGDragDropOperation>(InOperation);
	if (ensure(Op))
	{
		bIsR = Op->bIsRotated;
	}

	Inven->TryAddItemAt(InDragged->Inven, InDragged->EntryId, InDragged->ItemDef, InDragged->Quantity, Cell.X, Cell.Y, bIsR);
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
	W->SetupFromEntry(Inven, E, CellSize, CellPadding);
	W->bCanDrag = bCanDrag;
	return W;
}

void UATGInventoryGirdWidget::UpdateItemSlot(UATGInventoryItemWidget* W, const FInventoryEntry& E)
{
	if (!GridPanel || !W) return;
	UE_LOG(LogTemp, Warning, TEXT("UATGInventoryGirdWidget::UpdateItemSlot W : %d , H : %d"), int32(E.Width), int32(E.Height));

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
	if (!Inven) return nullptr;
	for (const FInventoryEntry& E : Inven->GetEntries())
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

bool UATGInventoryGirdWidget::CheckIsOutGrid(const FVector2D& Local) const
{
	if (Local.X < 0.f || Local.Y < 0.f)
	{
		return true;
	}
	const float Pitch = float(CellSize + 2 * CellPadding);
	float WSize = float(Pitch * Inven->GetGridWidth());
	float HSize = float(Pitch * Inven->GetGridHeight());
	if (Local.X > WSize || Local.Y > HSize)
	{
		return true;
	}

	return false;
}

void UATGInventoryGirdWidget::DoNativeOnDrop(UDragDropOperation* InOperation, UATGInventoryItemWidget* Dragged, FVector2D Screen)
{
	const FGeometry PanelGeo = GridPanel->GetTickSpaceGeometry();

	const FVector2D Local = PanelGeo.AbsoluteToLocal(Screen);

	if (bIsDragLeave/*CheckIsOutGrid(Local)*/)
	{
		Inven->TryDropItem(Dragged->EntryId);
		return;
	}

	FIntPoint Cell = CellFromLocal(Local);

	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("Cell")+ Cell.ToString());
	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("Local") + Local.ToString());

	// 안전 클램프(서버도 판정하지만 UX용으로 선제 클램프)
	Cell.X = FMath::Clamp(Cell.X, 0, Inven->GetGridWidth() - 1);
	Cell.Y = FMath::Clamp(Cell.Y, 0, Inven->GetGridHeight() - 1);

	bool bShouldRotate = false;
	bool bIsR = false;
	UATGDragDropOperation* Op = Cast<UATGDragDropOperation>(InOperation);
	if (ensure(Op))
	{
		bIsR = Op->bIsRotated;
	}

	const FInventoryEntry* E = Inven->GetInventory().GetById(Dragged->EntryId);
	if (E)
	{
		
		bShouldRotate = E->bRotated ? !bIsR : bIsR;
	}

	//UE_LOG(LogTemp, Warning, TEXT("bIsR : %d"), bIsR);
	//InventoryComp->ServerMoveOrSwap(Dragged->EntryId, Cell.X, Cell.Y, bIsRKeyPressed);
	Inven->TryMoveOrSwapClient(Dragged->EntryId, Cell.X, Cell.Y, bShouldRotate);

	return;
}

// Split Version Overload
void UATGInventoryGirdWidget::DoNativeOnDrop(UDragDropOperation* InOperation, UATGInventoryItemWidget* Dragged, FVector2D Screen, int32 SplitNum)
{
	const FGeometry PanelGeo = GridPanel->GetTickSpaceGeometry();

	const FVector2D Local = PanelGeo.AbsoluteToLocal(Screen);

	if (CheckIsOutGrid(Local)) // 분할시 UI조작하면 그리드 밖으로 나가기 때문에 좌표로 판정
	{
		Inven->TryDropItem(Dragged->EntryId, SplitNum);
		return;
	}

	FIntPoint Cell = CellFromLocal(Local);

	Cell.X = FMath::Clamp(Cell.X, 0, Inven->GetGridWidth() - 1);
	Cell.Y = FMath::Clamp(Cell.Y, 0, Inven->GetGridHeight() - 1);

	bool bShouldRotate = false;
	bool bIsR = false;
	UATGDragDropOperation* Op = Cast<UATGDragDropOperation>(InOperation);
	if (ensure(Op))
	{
		bIsR = Op->bIsRotated;
	}

	const FInventoryEntry* E = Inven->GetInventory().GetById(Dragged->EntryId);
	if (E)
	{

		bShouldRotate = E->bRotated ? !bIsR : bIsR;
	}

	Inven->TrySplitStack(Dragged->EntryId, Cell.X, Cell.Y, bShouldRotate, SplitNum);

	return;
}

bool UATGInventoryGirdWidget::NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	//Super::NativeOnDrop(InGeo, InDragDropEvent, InOperation);
	// !bCanDrag 일때 다른 그리드에서 아이템 받기 안됨
	if (!bCanDrag)
	{
		return false;
	}

	UE_LOG(LogTemp, Display,TEXT("UATGInventoryGirdWidget::NativeOnDrop"));

	// 이 스코프 끝날 때 무조건 호출됨
	ON_SCOPE_EXIT
	{
		SetAllGridDefault();
	};

	if (!Inven || !GridPanel)
	{
		return true;
	}

	if (UATGInventoryItemWidget* Dragged = InOperation ? Cast<UATGInventoryItemWidget>(InOperation->Payload) : nullptr)
	{

		const FVector2D Screen = InDragDropEvent.GetScreenSpacePosition();
		
		if (CheckIsFromOther(Dragged))	//from other grid and drag entered
		{
			UE_LOG(LogTemp, Warning, TEXT("this Item is Not Contain Current Grid"));

			HandleIncomingItem(InOperation, Dragged, Screen);

			return true;
		}
		else if (Dragged->Inven != Inven)	//from other grid and not drag entered
		{
			return true;
		}

		//from my grid
		//when controlkey down split stack
		if (InDragDropEvent.IsControlDown() && Dragged->Quantity != 1)
		{
			if (!SplitUI)
			{
				SplitUI = CreateWidget<UATGStackSplitWidget>(GetWorld(), StackSplitWidgetClass);
		
				SplitUI->AddToViewport();
			}
			SplitUI->OnSplitConfirmed.Clear();
			SplitUI->OnSplitConfirmed.AddLambda([this, Dragged, Screen, InOperation](int32 SplitNum)
				{
					//서버에 분할 요청
					DoNativeOnDrop(InOperation, Dragged, Screen, SplitNum);
				});

			SplitUI->SetVisibility(ESlateVisibility::Visible);
			
			//int32 Qty = 1;
			//LexTryParseString(Qty, *Dragged->QuantityText->GetText().ToString());
			SplitUI->InitSplit(Dragged->Quantity);
			
			return true;
		}

		//when just drop
		DoNativeOnDrop(InOperation, Dragged, Screen);

		return true;
	}

	return true;
}


bool UATGInventoryGirdWidget::CheckIsFromOther(UATGInventoryItemWidget* Dragged)
{
	return (Dragged->Inven != Inven && !bIsDragLeave);
}

void UATGInventoryGirdWidget::NativeOnDragEnter(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	//Super::NativeOnDragEnter(InGeo, InDragDropEvent, InOperation);
	// TODO: 클라 미리보기(가능/불가 하이라이트) 구현 시 여기서 셀 강조 처리
	
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("NativeOnDragEnter"));
	bIsDragLeave = false;
}
 

void UATGInventoryGirdWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	//Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	// TODO: 하이라이트 해제
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("NativeOnDragLeave"));
	SetAllGridDefault();
	
	//Operation = nullptr;
	//bIsRotate = false;
}

bool UATGInventoryGirdWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	//NET_LOG(TEXT(""));
	if (!Inven || !GridPanel) return false;

	if (UATGInventoryItemWidget* Dragged = InOperation ? Cast<UATGInventoryItemWidget>(InOperation->Payload) : nullptr)
	{

		const FVector2D Screen = InDragDropEvent.GetScreenSpacePosition();

		const FGeometry PanelGeo = GridPanel->GetTickSpaceGeometry();

		const FVector2D Local = PanelGeo.AbsoluteToLocal(Screen);

		FIntPoint Cell = CellFromLocal(Local);

		//if (GEngine)
		//	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("Cell")+ Cell.ToString());
		//if (GEngine)
		//	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("Local") + Local.ToString());

		// 안전 클램프(서버도 판정하지만 UX용으로 선제 클램프)
		Cell.X = FMath::Clamp(Cell.X, 0, Inven->GetGridWidth() - 1);
		Cell.Y = FMath::Clamp(Cell.Y, 0, Inven->GetGridHeight() - 1);

		if (bIsDragLeave/*CheckIsOutGrid(Local)*/)
		{
			//SetAllGridDefault();
			return false;
		}

		//같은 칸이면 넘김
		if (PrevCell == Cell)
		{
			return false;
		}
		PrevCell = Cell;

		UATGDragDropOperation* Op = Cast<UATGDragDropOperation>(InOperation);
		ensure(Op);

		int32 W = 0;
		int32 H = 0;
		int32 IgnoreId = -1;

		const FInventoryEntry* E = Inven->GetInventory().GetById(Dragged->EntryId);
		if (!E)
		{
			UE_LOG(LogTemp, Error, TEXT("Dragged->Entry is invalid!"));

			if (CheckIsFromOther(Dragged))
			{
				UATGItemData* ItemData = Dragged->ItemDef.Get();
				if (!ItemData)
				{
					ItemData = Dragged->ItemDef.LoadSynchronous();
				}

				W = Op->bIsRotated ? ItemData->Height : ItemData->Width;
				H = Op->bIsRotated ? ItemData->Width : ItemData->Height;
			}
			else
			{
				return false;
			}
		}
		else
		{
			IgnoreId = E->Id;

			if (E->bRotated)
			{
				W = Op->bIsRotated ? E->Width : E->Height;
				H = Op->bIsRotated ? E->Height : E->Width;
			}
			else
			{
				W = Op->bIsRotated ? E->Height : E->Width;
				H = Op->bIsRotated ? E->Width : E->Height;
			}
		}

		bool bCanMove = Inven->CheckCanMove(Cell.X, Cell.Y, W, H, IgnoreId);
		FString s = bCanMove ? TEXT("True") : TEXT("False");
		NET_LOG(FString::Printf(TEXT("%s %d %d %d %d"), *s, Cell.X, Cell.Y, W, H));
		/*if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, FString::Printf(TEXT("W : %d H : %d ID : %d"), W, H, E->Id));
		}*/

		for (auto Child : GridPanel->GetAllChildren())
		{
			USizeBox* CellBox = Cast<USizeBox>(Child);
			if (!CellBox)
			{
				continue;
			}

			if (!Child || !Child->Slot)
			{
				continue;
			}
				
			if (UGridSlot* CellSlot = Cast<UGridSlot>(Child->Slot))
			{
				bool bIsTargetGrid = false;
				for (int X = 0; X < W; ++X)
				{
					for (int Y = 0; Y < H; ++Y)
					{
						if (CellSlot->GetColumn() == Cell.X+X && CellSlot->GetRow() == Cell.Y+Y)
						{
							bIsTargetGrid = true;
							
							for (auto WG : CellBox->GetAllChildren())
							{
								if (UImage* Img = Cast<UImage>(WG))
								{
									FLinearColor PreviewColor = bCanMove ? CheckTrueColor : CheckFalseColor;
									Img->SetColorAndOpacity(PreviewColor);
									CellSlot->SetLayer(1);
									break;
								}
							}
						
							break;
						}
					}
					if (bIsTargetGrid)
					{
						break;
					}
				}
				if (bIsTargetGrid)
				{
					continue;
				}
				
				for (auto WG : CellBox->GetAllChildren())
				{
					if (UImage* Img = Cast<UImage>(WG))
					{
						Img->SetColorAndOpacity(DefaultColor);
						CellSlot->SetLayer(0);
						break;
					}
				}
			}
		}

		return false;
	}

	return false;
}

void UATGInventoryGirdWidget::SetAllGridDefault()
{
	bIsDragLeave = true;
	PrevCell = FIntPoint(-1);
	for (auto Child : GridPanel->GetAllChildren())
	{
		if (USizeBox* CellBox = Cast<USizeBox>(Child))
		{
			for (auto WG : CellBox->GetAllChildren())
			{
				if (UImage* Img = Cast<UImage>(WG))
				{
					Img->SetColorAndOpacity(DefaultColor);
					UGridSlot* CellSlot = Cast<UGridSlot>(Child->Slot);
					if (CellSlot) CellSlot->SetLayer(0);
					break;
				}
			}
		}
	}
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
	HandleItemPreRemoved(EntryId); //프리뷰 제거
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

void UATGInventoryGirdWidget::HandleRebuildAll(int32 EntryId)
{
	RebuildAll();
}

//void UATGInventoryGirdWidget::HandleItemRotated(int32 EntryId)
//{
//	if (UWidget* Ghost = Operation ? Operation->DefaultDragVisual : nullptr)
//	{
//		//90도 시각 회전
//		Ghost->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
//		FWidgetTransform T = Ghost->GetRenderTransform();
//
//		T.Angle = bIsRKeyPressed ? T.Angle += 90.f : T.Angle -= 90.f;
//
//		Ghost->SetRenderTransform(T);
//	}
//}

void UATGInventoryGirdWidget::HandleItemPreAdded(FInventoryEntry PreE)
{

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("HandleItemPreAdded"));
	UATGInventoryItemWidget* W = CreateItemWidget(PreE);
	if (!W)
	{
		return;
	}
	W->SetVisibility(ESlateVisibility::HitTestInvisible); //preview widget 상호작용 불가
	W->ItemIcon->SetColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 0.5f));
	UpdateItemSlot(W, PreE);
	
	PreviewIdToWidget.FindOrAdd(PreE.Id).Add(W); //배열맵에 키값 같은것 끼리 저장
}

//클라에서 실행 프리뷰위젯 수량 텍스트 변경
void UATGInventoryGirdWidget::HandleItemPreChanged(FInventoryEntry PreE)
{
	if (TArray<TWeakObjectPtr<UATGInventoryItemWidget>>* Arr = PreviewIdToWidget.Find(PreE.Id))
	{
		for (auto& WeakW : *Arr)
		{
			if (UATGInventoryItemWidget* W = WeakW.Get())
			{
				W->RefreshFromEntry(PreE, CellSize, CellPadding);
			}
		}
	}
}

void UATGInventoryGirdWidget::HandleItemPreRemoved(int32 PreEId)
{
	if (TArray<TWeakObjectPtr<UATGInventoryItemWidget>>* Arr = PreviewIdToWidget.Find(PreEId))
	{
		for (auto& WeakW : *Arr)
		{
			if (UATGInventoryItemWidget* W = WeakW.Get())
			{
				W->RemoveFromParent();
			}
		}

		PreviewIdToWidget.Remove(PreEId);
	}
}

void UATGInventoryGirdWidget::OnSortBtnClicked()
{
	if (Inven)
	{
		Inven->TrySortByItemId();
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("UATGInventoryGirdWidget::OnSortBtnClicked Inven == nullptr"));
	}
}
