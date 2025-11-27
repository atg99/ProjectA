// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/ATGEquipmentGirdWidget.h"
#include "ATGEquipmentComponent.h"
#include "ATGInventoryItemWidget.h"
#include "Data/ATGItemData.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "ATGDragDropOperation.h"
#include "Blueprint/DragDropOperation.h"
#include "Data/ATGEquipmentData.h"
#include "Data/ATGWeaponData.h"

void UATGEquipmentGirdWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UATGEquipmentGirdWidget::BindInventoryComp()
{
	UATGEquipmentComponent* EquipmentComp = Cast<UATGEquipmentComponent>(Inven.GetObject());
	
	if (!EquipmentComp)
	{
		ensure(EquipmentComp);
		return;
	}
	
	//자신의 장비 슬롯 유형에 맞는 이벤트 구독 (장비슬롯은 이 함수 호출전에 부모위젯 InventoryWidget에서 주입됨)
	switch (EquipmentSlot)
	{
	case EEquipmentSlotType::None:
		break;
	case EEquipmentSlotType::MainWeapon1:
		EquipmentComp->OnFirstMainWeaponChanged.AddDynamic(this, &UATGEquipmentGirdWidget::HandleEquipmentChanged);

		FitEquipmentType = EEquipmentType::Weapon;
		FitWeaponType = EWeaponType::MainWeapon;
		break;
	case EEquipmentSlotType::MainWeapon2:
		EquipmentComp->OnSecondMainWeaponChanged.AddDynamic(this, &UATGEquipmentGirdWidget::HandleEquipmentChanged);

		FitEquipmentType = EEquipmentType::Weapon;
		FitWeaponType = EWeaponType::MainWeapon;
		break;
	}
	
	RebuildAll();
}

void UATGEquipmentGirdWidget::HandleEquipmentChanged(FInventoryEntry EquipmentEntry)
{
	//받은 Entry정보대로 위젯 변경
	//Item null 이면 Widget 삭제
	if (!EquipmentEntry.Item)
	{
		if (EquipmentWidget)
		{
			EquipmentWidget->RemoveFromParent();
		}
		return;
	}
	
	//유효하면 생성 or 변경
	if (!EquipmentWidget)
	{
		UATGInventoryItemWidget* W = CreateItemWidget(EquipmentEntry);
		EquipmentWidget = W;
	}
	
	EquipmentWidget->RefreshFromEntry(EquipmentEntry, CellSize, CellPadding);
	UpdateItemSlot(EquipmentWidget, EquipmentEntry);
}

void UATGEquipmentGirdWidget::RebuildAll()
{
	if (!GridPanel || !Inven) return;

	GridPanel->ClearChildren();
	GridPanel->InvalidateLayoutAndVolatility(); // 레이아웃 새로 계산 강제

	IdToWidget.Empty();

	BuildCellBackground();

	FInventoryEntry Equipment;
	Inven->GetEquipmentEntry(EquipmentSlot, Equipment);

	//데이터가 있을 경우만 
	if (Equipment.Item)
	{
		//장비 그리드좌표 0,0 고정
		UATGInventoryItemWidget* W = CreateItemWidget(Equipment);
		UpdateItemSlot(W, Equipment);
		EquipmentWidget = W;
	}
}

void UATGEquipmentGirdWidget::BuildCellBackground()
{
	if (!GridPanel || !Inven) return;
	int32 W = 0;
	int32 H = 0;

	UATGEquipmentComponent* EquipmentComp = Cast<UATGEquipmentComponent>(Inven.GetObject());
	if (!EquipmentComp)
	{
		return;
	}

	switch (EquipmentSlot)
	{
	case EEquipmentSlotType::MainWeapon1:
		W = EquipmentComp->WeaponSlotSize.X;
		H = EquipmentComp->WeaponSlotSize.Y;
		UE_LOG(LogTemp, Warning, TEXT("!!! EEquipmentSlotType::MainWeapon1"));
		break;
	case EEquipmentSlotType::MainWeapon2:
		W = EquipmentComp->WeaponSlotSize.X;
		H = EquipmentComp->WeaponSlotSize.Y;
		UE_LOG(LogTemp, Warning, TEXT("!!! EEquipmentSlotType::MainWeapon2"));
		break;
	case EEquipmentSlotType::None:
		UE_LOG(LogTemp, Warning, TEXT("!!! EEquipmentSlotType is None"));
		return;
	}

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

void UATGEquipmentGirdWidget::HandleIncomingItem(UDragDropOperation* InOperation, UATGInventoryItemWidget* InDragged, FVector2D Screen)
{
	if (!InDragged || !InDragged->ItemDef) return;
	UE_LOG(LogTemp, Display, TEXT("UATGEquipmentGirdWidget::HandleIncomingItem"));

	if (UATGItemData* ItemData = InDragged->ItemDef.Get())
	{
		if (CheckFitEquip(ItemData))
		{
			// 슬롯 타입 전달 (X좌표에 Enum 값 할당)
			Inven->TryAddItemAt(InDragged->Inven, InDragged->EntryId, InDragged->ItemDef, InDragged->Quantity, (int32)EquipmentSlot, 0);
		}
	}
}

bool UATGEquipmentGirdWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!Inven || !GridPanel) return false;

	if (UATGInventoryItemWidget* Dragged = InOperation ? Cast<UATGInventoryItemWidget>(InOperation->Payload) : nullptr)
	{
		const FVector2D Screen = InDragDropEvent.GetScreenSpacePosition();
		const FGeometry PanelGeo = GridPanel->GetTickSpaceGeometry();
		const FVector2D Local = PanelGeo.AbsoluteToLocal(Screen);

		FIntPoint Cell = CellFromLocal(Local);

		// 안전 클램프
		Cell.X = FMath::Clamp(Cell.X, 0, Inven->GetGridWidth() - 1);
		Cell.Y = FMath::Clamp(Cell.Y, 0, Inven->GetGridHeight() - 1);

		if (bIsDragLeave)
		{
			return false;
		}

		//같은 칸이면 넘김
		if (PrevCell == Cell)
		{
			return false;
		}
		PrevCell = Cell;

		const FInventoryEntry* E = Dragged->Inven->GetInventory().GetById(Dragged->EntryId);
		if (!E)
		{
			return false;
		}
	
		bool bIsR = false;
		UATGDragDropOperation* Op = Cast<UATGDragDropOperation>(InOperation);
		if (ensure(Op))
		{
			bIsR = Op->bIsRotated;
		}
		
		int32 W = bIsR ? E->Height : E->Width;
		int32 H = bIsR ? E->Width : E->Height;
		
		bool bCanMove = Inven->CheckCanMove(Cell.X, Cell.Y, W, H, E->Id);

		if (UATGItemData* ItemData = E->Item.Get())
		{
			if (!CheckFitEquip(ItemData))
			{
				bCanMove = false;
			}
		}
		
		for (auto Child : GridPanel->GetAllChildren())
		{
			USizeBox* CellBox = Cast<USizeBox>(Child);
			if (!CellBox) continue;

			if (!Child || !Child->Slot) continue;
				
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
					if (bIsTargetGrid) break;
				}
				if (bIsTargetGrid) continue;
				
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

bool UATGEquipmentGirdWidget::CheckIsFromOther(UATGInventoryItemWidget* Dragged)
{

	return (Dragged->Inven != Inven && !bIsDragLeave) || (Dragged->Inven == Inven && !bIsDragLeave);
}

bool UATGEquipmentGirdWidget::CheckFitEquip(UATGItemData* ItemData)
{
	if (!ItemData)
	{
		return false;
	}

	//장비인가?
	if (UATGEquipmentData* EquipData = Cast<UATGEquipmentData>(ItemData))
	{
		//장비 타입이 슬롯과 일치?
		if (EquipData->EquipmentType == FitEquipmentType)
		{
			//장비 타입의 세부 타입이 슬롯과 일치?
			switch (FitEquipmentType)
			{
			case EEquipmentType::Weapon:
			{
				UATGWeaponData* WeaponData = Cast<UATGWeaponData>(EquipData);
				return WeaponData ? WeaponData->WeaponType == FitWeaponType : false;
			}
			case EEquipmentType::Armor:
				break;//구현 예정
			default:
				break;
			}
		}
	}

	return false;
}
