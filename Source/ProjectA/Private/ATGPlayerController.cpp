// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "ATGPlayerState.h"

#include "GameFramework/PlayerState.h"
#include "ATGInventoryComponent.h"
#include "GameFramework/HUD.h"
#include "ATGHUDComponent.h"
#include "ATGHUDWidget.h"
#include "ATGInventoryWidget.h"
#include "ATGInventoryGirdWidget.h"
#include "Utils/NetworkUtil.h"

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
				Subsystem->AddMappingContext(CurrentContext, 1);
			}

			if (MotionMatcingContexts)
			{
				Subsystem->AddMappingContext(MotionMatcingContexts, 0);
			}
		}
	}
}

void AATGPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}

	//리슨서버 위젯생성
	if (HasAuthority() && IsLocalController())
	{
		StartInitInventoryWidget();
	}
}

void AATGPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	//GridWidget이 PlayerState의 InventoryComponet가 필요하므로 클라는 PlayeerState가 복제되는 시점에 위젯 생성
	UE_LOG(LogTemp, Error, TEXT("OnRep_PlayerStateOnRep_PlayerState"));
	StartInitInventoryWidget();
}

void AATGPlayerController::StartInitInventoryWidget()
{
	if (!IsLocalController()) return;
	if (!GetHUD())
	{
		UE_LOG(LogTemp, Error, TEXT("Can't Find HUD"));
		return;
	}

	InvenComp = GetPlayerState<APlayerState>()->FindComponentByClass<UATGInventoryComponent>();
	if (InvenComp)
	{
		InitInventoryComponent.Broadcast(InvenComp);
		//HUDComp->HUDWidget->InventoryWidget->PlayerGrid->InventoryComp = Comp;
		//HUDComp->HUDWidget->InventoryWidget->PlayerGrid->BindInventoryComp();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Can't Find UATGInventoryComponent at PlayerState"));
	}

	UATGHUDComponent* HUDComp = GetHUD()->FindComponentByClass<UATGHUDComponent>();
	if (HUDComp)
	{
		HUDComp->EnsureWidgetCreated(this);
		//HUDComp->OnInventToggle.AddDynamic()
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Can't Find HUDComponent at HUD"));
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
				Subsystem->AddMappingContext(InventoryMappingContexts, 3);
			}
			else
			{
				Subsystem->RemoveMappingContext(InventoryMappingContexts);
			}
		}
	}
}

//총기 조작 input mapping
void AATGPlayerController::WeaponInputMapping(EEquipmentSlotType CurrentUsingSlot)
{
	if (IsLocalPlayerController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			NET_LOG(FString::Printf(TEXT("%d"), CurrentUsingSlot));
			if (CurrentUsingSlot == EEquipmentSlotType::MainWeapon1 || CurrentUsingSlot == EEquipmentSlotType::MainWeapon2)
			{
				Subsystem->AddMappingContext(GunWeaponMappingContexts, 2);
				Subsystem->RemoveMappingContext(MeleeWeaponMappingContexts);
			}
			else if(CurrentUsingSlot == EEquipmentSlotType::MeleeWeapon)
			{
				Subsystem->AddMappingContext(MeleeWeaponMappingContexts, 2);
				Subsystem->RemoveMappingContext(GunWeaponMappingContexts);
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
