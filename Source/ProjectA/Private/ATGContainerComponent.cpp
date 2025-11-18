// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGContainerComponent.h"
#include "ATGItemData.h"
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

	ContainerInventory.Owner = TScriptInterface<IATGInventoryOwnerInterface>(this);

	for (auto& Item : ContainerItems)
	{
		if (!Item.ItemDef.Get())
		{
			Item.ItemDef.LoadSynchronous();
		}
	}

	if (GetOwner()->HasAuthority())
	{
		InitContainerItem();
	}
	
	// ...
	//ContainerInventory.OwnerComp = this;
}

void UATGContainerComponent::InitContainerItem()
{
	if (!GetOwner()->HasAuthority()) return;

	//수정예정
	for (auto Item : ContainerItems)
	{
		int32 W = Item.ItemDef->Width;
		int32 H = Item.ItemDef->Height;
		int32 OutX = -1, OutY = -1;
		int32 Qty = Item.Quantity;

		if (!ContainerInventory.FindFirstFit(Item.ItemDef, W, H, OutX, OutY, Qty)) //여기서 존재하는 스택에 저장 남은 값 Qty 참조로 반환
		{
			continue; // 새로운 자리 없음 
		}

		if (Qty <= 0) //수량이 0이 된경우 
		{
			Item.Quantity = 0;
			continue;
		}

		ContainerInventory.AddItemAt(Item.ItemDef, Qty, OutX, OutY, W, H, false, -1);
		
		//Qty 참조 반환
		while (Qty >= 1) //수량이 0이 될때 까지 반복
		{
			OutX = -1;
			OutY = -1;
			if (!ContainerInventory.FindFirstFit(W, H, OutX, OutY)) //다시 자리 검색, 존재하는 스택 저장 X 
			{
				break;
			}
			int32 Id = ContainerInventory.AddItemAt(Item.ItemDef, Qty, OutX, OutY, W, H, false, -1);
			if (Id == 0)
			{
				break;
			}
		}

		Item.Quantity = Qty;
	}

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

void UATGContainerComponent::ItemRemoved(int32 EntryId)
{
	OnContainerRemoved.Broadcast(EntryId);
}

void UATGContainerComponent::ItemAdded(int32 EntryId)
{
	OnContainerAdded.Broadcast(EntryId);
}

void UATGContainerComponent::ItemChanged(int32 EntryId)
{
	OnContainerChanged.Broadcast(EntryId);
}

void UATGContainerComponent::InventoryForceNetUpdate()
{
	GetOwner()->ForceNetUpdate();
}

//사용안함 일단 남김
bool UATGContainerComponent::IsLocallyOwned()
{
	return false;
}

