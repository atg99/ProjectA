// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "ATGPlayerState.h"
#include "ATGInventoryGirdWidget.h"
#include "GameFramework/PlayerState.h"
#include "ATGInventoryComponent.h"


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
		EnsureWidgetCreated();
	}
}

void AATGPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (!IsLocalController()) return;

	if (!InventoryWidget)
	{
		EnsureWidgetCreated();
	}

	UATGInventoryComponent* Comp = GetPlayerState<APlayerState>()->FindComponentByClass<UATGInventoryComponent>();
	if (Comp)
	{
		InventoryWidget->InventoryComp = Comp;
		InventoryWidget->BindInventoryComp();
	}
}

void AATGPlayerController::EnsureWidgetCreated()
{
	if (InventoryWidget || !InventoryWidgetClass)
	{
		return;
	}
	else
	{

	}

	InventoryWidget = CreateWidget<UATGInventoryGirdWidget>(this, InventoryWidgetClass);
	if (InventoryWidget)
	{
		InventoryWidget->AddToViewport();
		InventoryWidget->SetVisibility(ESlateVisibility::Visible);
	}

	if (GEngine)
			GEngine->AddOnScreenDebugMessage(10, 3.0f, FColor::Magenta, TEXT("WidgetCreated"));
}
