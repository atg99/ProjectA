// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGWeaponBase.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AATGWeaponBase::AATGWeaponBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	
	SetRootComponent(StaticMeshComp);
	

}

// Called when the game starts or when spawned
void AATGWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AATGWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AATGWeaponBase::PlayerInteract(FInteractionData& InteractionData)
{
}

