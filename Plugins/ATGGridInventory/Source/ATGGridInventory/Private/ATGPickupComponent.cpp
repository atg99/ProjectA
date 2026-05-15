// Fill out your copyright notice in the Description page of Project Settings.

#include "ATGPickupComponent.h"
#include "ATGItem.h"

#include "Components/StaticMeshComponent.h"
#include "Data/ATGItemData.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

UATGPickupComponent::UATGPickupComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UATGPickupComponent::BeginPlay()
{
	Super::BeginPlay();

	SetItemMesh();
}

void UATGPickupComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UATGPickupComponent::PlayerInteract(FInteractionData& InteractionData)
{
	InteractionData.InteractedActor = GetOwner();
	InteractionData.InteractionType = InteractionType;
	InteractionData.ItemDef = ItemDef;
	InteractionData.ItemQty = ItemQty;
}

void UATGPickupComponent::DecreaseQty(int32 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (Amount < 0)
	{
		return;
	}

	ItemQty -= Amount;

	if (ItemQty <= 0)
	{
		GetOwner()->Destroy();
	}
}

void UATGPickupComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UATGPickupComponent, ItemDef, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(UATGPickupComponent, ItemQty, COND_None);
}

void UATGPickupComponent::SetItemMesh()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UStaticMeshComponent* ItemMeshComp = Owner->GetComponentByClass<UStaticMeshComponent>();
	if (!ItemMeshComp)
	{
		return;
	}

	if (!ItemDef.Get())
	{
		ItemDef.LoadSynchronous();
	}

	UATGItemData* Data = ItemDef.Get();
	if (!Data)
	{
		return;
	}

	if (AATGItem* ItemActor = Cast<AATGItem>(Owner))
	{
		ItemActor->ApplyItemDataMesh(Data);
	}
	else if (Data->Mesh)
	{
		ItemMeshComp->SetStaticMesh(Data->Mesh);
	}
}
