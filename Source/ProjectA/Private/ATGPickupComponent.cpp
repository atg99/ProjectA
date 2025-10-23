// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGPickupComponent.h"

// Sets default values for this component's properties
UATGPickupComponent::UATGPickupComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UATGPickupComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UATGPickupComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UATGPickupComponent::PlayerInteract(FInteractionData& InteractionData)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("PickupCompoent PlayerInteract"));
	InteractionData.InteractedActor = GetOwner();
	InteractionData.InteractionType = InteractionType;
	InteractionData.ItemDef = ItemDef;
}

