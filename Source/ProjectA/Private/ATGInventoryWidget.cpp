// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGInventoryWidget.h"
#include "ATGInventoryGirdWidget.h"
#include "ATGPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "ATGInventoryComponent.h"
#include "ATGContainerComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "ATGInventoryItemWidget.h"
#include "ATGHUDComponent.h"
#include "Components/GridPanel.h"
#include "ATGEquipmentComponent.h"
#include "GameFramework/PlayerState.h"
#include "../Public/Widget/ATGEquipmentGirdWidget.h"


void UATGInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//PlayerGrid->InitPlayerGrid();

	//SetKeyboardFocus();
	
	//UE_LOG(LogTemp, Log, TEXT("Keyboard focus set on widget"));
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APlayerState* PS = PC->PlayerState)
		{
			auto EquipComp = PS->FindComponentByClass<UATGEquipmentComponent>();
			auto InvenComp = PS->FindComponentByClass<UATGInventoryComponent>();

			InjectInvenComp(InvenComp, EquipComp);

			// 로비맵일때 PS에 ContainerComponent추가 DB data 연결 (게임맵 PS 에 ContainerComp 달면안됨)
			auto ContainerComp = PS->FindComponentByClass<UATGContainerComponent>();
			if (ContainerComp)
			{
				HandleContainerToggle(ContainerComp);
			}
		}
	}	

	//AATGPlayerController* PC = Cast<AATGPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	//if (PC)
	//{
	//	if (PC->InvenComp)
	//	{
	//		InjectInvenComp(PC->InvenComp);
	//	}
	//	else
	//	{
	//		PC->InitInventoryComponent.AddDynamic(this, &UATGInventoryWidget::HandleInitInventoryComp);
	//	}
	//}
}

//FReply UATGInventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
//{
//	if (InKeyEvent.GetKey() == EKeys::R)
//	{
//		return FReply::Handled();
//	}
//	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
//}
//
//FReply UATGInventoryWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
//{
//	if (InKeyEvent.GetKey() == EKeys::R)
//	{
//		return FReply::Handled();
//	}
//	return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
//}

void UATGInventoryWidget::SetHUDComp(UATGHUDComponent* InHUDComp)
{
	HUDComp = InHUDComp;

	if (HUDComp)
	{
		HUDComp->OnContainerToggle.AddDynamic(this, &UATGInventoryWidget::HandleContainerToggle);
	}
}

void UATGInventoryWidget::HandleContainerToggle(UATGContainerComponent* ContainerComp)
{
	if (ContainerComp)
	{
		ContainerGrid->Inven = ContainerComp;
		ContainerGrid->BindInventoryComp();
	}
}

void UATGInventoryWidget::HandleInitInventoryComp(UATGInventoryComponent* GetInventoryComponent)
{
	//InjectInvenComp(GetInventoryComponent);
}

void UATGInventoryWidget::InjectInvenComp(UATGInventoryComponent* InInventoryComponent, UATGEquipmentComponent* InEquipmentComponent)
{
	if (ensure(InInventoryComponent))
	{
		InventoryComponent = InInventoryComponent;
		PlayerGrid->Inven = InInventoryComponent;
		PlayerGrid->BindInventoryComp();
	}
	
	if (ensure(InEquipmentComponent))
	{
		EquipmentComponent = InEquipmentComponent;

		MainWeapon1Grid->EquipmentSlot = EEquipmentSlotType::MainWeapon1Slot;
		MainWeapon2Grid->EquipmentSlot = EEquipmentSlotType::MainWeapon2Slot;

		MainWeapon1Grid->Inven = InEquipmentComponent;
		MainWeapon2Grid->Inven = InEquipmentComponent;

		MainWeapon1Grid->BindInventoryComp();
		MainWeapon2Grid->BindInventoryComp();
	}
	
	//PlayerGrid->InventoryComp = GetInventoryComponent;
	
}

bool UATGInventoryWidget::NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UE_LOG(LogTemp, Display, TEXT("UATGInventoryWidget::NativeOnDrop"));

	const FVector2D Screen = InDragDropEvent.GetScreenSpacePosition();

	const FGeometry PlayerPanelGeo = PlayerGrid->GridPanel->GetTickSpaceGeometry();
	const FGeometry ContainerPanelGeo = ContainerGrid->GridPanel->GetTickSpaceGeometry();

	const FVector2D PlayerLocal = PlayerPanelGeo.AbsoluteToLocal(Screen);
	const FVector2D ContainerLocal = ContainerPanelGeo.AbsoluteToLocal(Screen);

	UE_LOG(LogTemp, Display, TEXT("PlayerLocal : %f, %f \n ContainerLocal : %f, %f"), PlayerLocal.X, PlayerLocal.Y, ContainerLocal.X, ContainerLocal.Y);

	PlayerGrid->NativeOnDrop(InGeo, InDragDropEvent, InOperation);
	ContainerGrid->NativeOnDrop(InGeo, InDragDropEvent, InOperation);

	//if (UATGInventoryItemWidget* Dragged = InOperation ? Cast<UATGInventoryItemWidget>(InOperation->Payload) : nullptr)
	//{
	//	InventoryComponent->TryDropItem(Dragged->EntryId);
	//}

	return false;
}

bool UATGInventoryWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	return false;
}

void UATGInventoryWidget::NativeOnDragEnter(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeo, InDragDropEvent, InOperation);
}

void UATGInventoryWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
}

void UATGInventoryWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
}

void UATGInventoryWidget::TogglePlayerGrid(bool bIsVisibie)
{
	if (!PlayerGrid) return;

	if (bIsVisibie)
	{
		PlayerGrid->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		PlayerGrid->SetVisibility(ESlateVisibility::Collapsed);
	}
}
