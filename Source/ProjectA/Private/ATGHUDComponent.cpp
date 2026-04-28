// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGHUDComponent.h"
#include "ATGInventoryGirdWidget.h"
#include "ATGHUDWidget.h"
#include "ATGInventoryWidget.h"
#include "GameFramework/HUD.h"
#include "ATGPlayerController.h"

// Sets default values for this component's properties
UATGHUDComponent::UATGHUDComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UATGHUDComponent::BeginPlay()
{
	Super::BeginPlay();
	// ...
	
}


// Called every frame
void UATGHUDComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UATGHUDComponent::EnsureWidgetCreated(APlayerController* PC)
{
	if (!HUDWidget && !HUDWidgetClass)
	{
		return;
	}

	if (!HUDWidget)
	{
		HUDWidget = CreateWidget<UATGHUDWidget>(PC, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->HUDComp = this;
			HUDWidget->AddToViewport();
			HUDWidget->SetVisibility(ESlateVisibility::Visible);
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(10, 3.0f, FColor::Magenta, TEXT("WidgetCreated"));
		}
	}

	if (HUDWidget)
	{
		HUDWidget->HUDComp = this;
		if (HUDWidget->InventoryWidget)
		{
			HUDWidget->InventoryWidget->SetHUDComp(this);
			HUDWidget->InventoryWidget->RefreshInventoryBindingsFromPlayerState();
		}
	}
}

void UATGHUDComponent::ToggleInventoryUI()
{
	if (!HUDWidget)
	{
		return;
	}

	bInvenVisible = !bInvenVisible;

	OnInventToggle.Broadcast(bInvenVisible);

	if (bInvenVisible)
	{
		GetHUDOwnerPC()->SetShowMouseCursor(true);
	
		Cast<AATGPlayerController>(GetHUDOwnerPC())->ToggleInventoryInputMapping(true);
		
	}
	else
	{
		GetHUDOwnerPC()->SetShowMouseCursor(false);
		
		Cast<AATGPlayerController>(GetHUDOwnerPC())->ToggleInventoryInputMapping(false);
		
	}
}


APlayerController* UATGHUDComponent::GetHUDOwnerPC() const
{
	return Cast<AHUD>(GetOwner())->GetOwningPlayerController();
}
