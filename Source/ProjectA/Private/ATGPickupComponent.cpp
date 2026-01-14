// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGPickupComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Data/ATGItemData.h"

// Sets default values for this component's properties
UATGPickupComponent::UATGPickupComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
	// ...
}


// Called when the game starts
void UATGPickupComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	SetItemMesh();
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
	InteractionData.ItemQty = ItemQty;
}

void UATGPickupComponent::DecreaseQty(int32 Amount)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	if (Amount < 0)
	{
		return;
	}

	//수량 감소
	ItemQty -= Amount;

	if (ItemQty <= 0)
	{
		GetOwner()->Destroy();
	}
}

void UATGPickupComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Iris 직접 markdirty해서 성능 최적화
	FDoRepLifetimeParams RepParams;
	RepParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(UATGPickupComponent, TestInt, RepParams);
	
	DOREPLIFETIME_CONDITION(UATGPickupComponent, ItemDef, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(UATGPickupComponent, ItemQty, COND_None);
}

void UATGPickupComponent::SetItemMesh()
{
	TestInt++;
	MARK_PROPERTY_DIRTY_FROM_NAME(UATGPickupComponent, TestInt, this);
	if (!GetOwner())
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("No Owner ATGPickupComp"));
		return;
	}
	UStaticMeshComponent* ItemMeshComp = GetOwner()->GetComponentByClass<UStaticMeshComponent>();
	if (ItemMeshComp)
	{	
		if (!ItemDef.Get())
		{
			ItemDef.LoadSynchronous();
		}
		UATGItemData* Data = ItemDef.Get();
		bool b = GetOwner()->HasAuthority();
		if (Data)
		{
			if (GEngine)
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("LoadData ATGPickupComp ") + LexToString(b));
			if (Data->Mesh)
			{
				ItemMeshComp->SetStaticMesh(Data->Mesh);
			}
		}
		else
		{
			if (GEngine)
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Can't LoadData ATGPickupComp ") + LexToString(b));
		}
	}

}

