// Fill out your copyright notice in the Description page of Project Settings.

#include "ATGItem.h"

#include "ATGPickupComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/ATGItemData.h"

AATGItem::AATGItem()
{
	PrimaryActorTick.bCanEverTick = true;

	ItemMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMeshComp);

	PickupComp = CreateDefaultSubobject<UATGPickupComponent>(TEXT("PickupComponent"));

	bReplicates = true;
}

void AATGItem::BeginPlay()
{
	Super::BeginPlay();
}

void AATGItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AATGItem::ApplyItemDataMesh(UATGItemData* ItemData)
{
	if (!bApplyItemDataMesh || !ItemMeshComp || !ItemData)
	{
		return;
	}

	if (ItemData->Mesh)
	{
		ItemMeshComp->SetStaticMesh(ItemData->Mesh);
	}

	if (bSimulatePhysicsAfterMeshApplied)
	{
		ItemMeshComp->SetMobility(EComponentMobility::Movable);
		ItemMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	ItemMeshComp->SetSimulatePhysics(bSimulatePhysicsAfterMeshApplied);
}
