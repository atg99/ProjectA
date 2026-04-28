// Fill out your copyright notice in the Description page of Project Settings.

#include "ATGItem.h"

#include "ATGPickupComponent.h"

AATGItem::AATGItem()
{
	PrimaryActorTick.bCanEverTick = true;

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
