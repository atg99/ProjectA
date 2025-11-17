// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGInventoryWidget.h"
#include "ATGInventoryGirdWidget.h"
#include "ATGPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "ATGInventoryComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "ATGInventoryItemWidget.h"


void UATGInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//PlayerGrid->InitPlayerGrid();

	AATGPlayerController* PC = Cast<AATGPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (PC)
	{
		if (PC->InvenComp)
		{
			InjectInvenComp(PC->InvenComp);
		}
		else
		{
			PC->InitInventoryComponent.AddDynamic(this, &UATGInventoryWidget::HandleInitInventoryComp);
		}
	}
}

void UATGInventoryWidget::HandleInitInventoryComp(UATGInventoryComponent* GetInventoryComponent)
{
	InjectInvenComp(GetInventoryComponent);
}

void UATGInventoryWidget::InjectInvenComp(UATGInventoryComponent* GetInventoryComponent)
{
	InventoryComponent = GetInventoryComponent;
	PlayerGrid->InventoryComp = GetInventoryComponent;
	PlayerGrid->BindInventoryComp();
}

bool UATGInventoryWidget::NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UE_LOG(LogTemp, Display, TEXT("UATGInventoryWidget::NativeOnDrop"));

	PlayerGrid->NativeOnDrop(InGeo, InDragDropEvent, InOperation);

	//if (UATGInventoryItemWidget* Dragged = InOperation ? Cast<UATGInventoryItemWidget>(InOperation->Payload) : nullptr)
	//{
	//	InventoryComponent->TryDropItem(Dragged->EntryId);
	//}

	return false;
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
