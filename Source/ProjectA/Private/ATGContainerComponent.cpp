// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGContainerComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UATGContainerComponent::UATGContainerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UATGContainerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	//ContainerInventory.OwnerComp = this;
}


// Called every frame
void UATGContainerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UATGContainerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UATGContainerComponent, ContainerInventory, COND_None);
}

void UATGContainerComponent::PlayerInteract(FInteractionData& InteractionData)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("UATGContainerComponent PlayerInteract"));
	InteractionData.InteractedActor = GetOwner();
	InteractionData.InteractedComponent = this;
	InteractionData.InteractionType = InteractionType;
}

