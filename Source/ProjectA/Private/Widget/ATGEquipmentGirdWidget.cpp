// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/ATGEquipmentGirdWidget.h"
#include "ATGEquipmentComponent.h"
#include "ATGInventoryItemWidget.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"


void UATGEquipmentGirdWidget::NativeConstruct()
{
	BuildCellBackground();
}

void UATGEquipmentGirdWidget::BindInventoryComp()
{
	RebuildAll();
}

void UATGEquipmentGirdWidget::RebuildAll()
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
}

void UATGEquipmentGirdWidget::BuildCellBackground()
{
	if (!GridPanel || !Inven) return;


	int32 W = 0;
	int32 H = 0;

	switch (EquipmentSlot)
	{
	case EEquipmentSlotType::MainWeapon1:
		W = 2; H = 1;
		UE_LOG(LogTemp, Warning, TEXT("!!! EEquipmentSlotType::MainWeapon1"));
		break;
	case EEquipmentSlotType::MainWeapon2:
		W = 2; H = 1;
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
