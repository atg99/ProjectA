// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "ATGPlayerState.h"
#include "ATGInventoryGirdWidget.h"
#include "GameFramework/PlayerState.h"
#include "ATGInventoryComponent.h"
#include "GameFramework/HUD.h"
#include "ATGHUDComponent.h"


void AATGPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}
		}
	}
}

void AATGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		
		
	}
}

void AATGPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (!IsLocalController()) return;
	if (!GetHUD())
	{
		UE_LOG(LogTemp, Error, TEXT("Can't Find HUD"));
		return;
	}
	UATGHUDComponent* HUDComp = GetHUD()->FindComponentByClass<UATGHUDComponent>();
	if (HUDComp)
	{
		HUDComp->EnsureWidgetCreated(this);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Can't Find HUDComponent at HUD"));
	}

	UATGInventoryComponent* Comp = GetPlayerState<APlayerState>()->FindComponentByClass<UATGInventoryComponent>();
	if (Comp)
	{
		HUDComp->InventoryWidget->InventoryComp = Comp;
		HUDComp->InventoryWidget->BindInventoryComp();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Can't Find UATGInventoryComponent at PlayerState"));
	}
}

//void AATGPlayerController::EnsureWidgetCreated()
//{
//	if (InventoryWidget || !InventoryWidgetClass)
//	{
//		return;
//	}
//	else
//	{
//
//	}
//
//	InventoryWidget = CreateWidget<UATGInventoryGirdWidget>(this, InventoryWidgetClass);
//	if (InventoryWidget)
//	{
//		InventoryWidget->AddToViewport();
//		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
//	}
//
//	if (GEngine)
//			GEngine->AddOnScreenDebugMessage(10, 3.0f, FColor::Magenta, TEXT("WidgetCreated"));
//}

void AATGPlayerController::ToggleInventoryInputMapping(bool bIsInvent)
{
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			if (bIsInvent)
			{
				Subsystem->AddMappingContext(InventoryMappingContexts, 1);
			}
			else
			{
				Subsystem->RemoveMappingContext(InventoryMappingContexts);
			}
		}
	}
}

//void AATGPlayerController::ToggleInventoryUI()
//{
//	if (!InventoryWidget)
//	{
//		return;
//	}
//	switch (InventoryWidget->GetVisibility())
//	{
//	case ESlateVisibility::Visible:
//		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
//		break;
//	case ESlateVisibility::Collapsed:
//		InventoryWidget->SetVisibility(ESlateVisibility::Visible);
//		break;
//	default:
//		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
//		break;
//	} 
//
//	if (InventoryWidget->GetVisibility() == ESlateVisibility::Visible)
//	{
//		SetShowMouseCursor(true);
//		ToggleInventoryInputMapping(true);
//	}
//	else
//	{
//		SetShowMouseCursor(false);
//		ToggleInventoryInputMapping(false);
//	}
//}
