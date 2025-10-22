// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGItem.h"
#include "ATGPickupComponent.h"

// Sets default values
AATGItem::AATGItem()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PickupCompoent = CreateDefaultSubobject<UATGPickupComponent>(TEXT("PickupComponent"));

}

// Called when the game starts or when spawned
void AATGItem::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AATGItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

