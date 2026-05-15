// Fill out your copyright notice in the Description page of Project Settings.

#include "ATGContainerComponent.h"
#include "ATGItem.h"
#include "ATGPickupComponent.h"
#include "Data/ATGItemData.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Net/Core/PushModel/PushModel.h"

UATGContainerComponent::UATGContainerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UATGContainerComponent::BeginPlay()
{
	Super::BeginPlay();

	ContainerInventory.GridWidth = GridSize.X;
	ContainerInventory.GridHeight = GridSize.Y;
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
}

void UATGContainerComponent::OnRegister()
{
	Super::OnRegister();
	ContainerInventory.Owner = TScriptInterface<IATGInventoryOwnerInterface>(this);
}

void UATGContainerComponent::InitContainerItem()
{
	if (!GetOwner()->HasAuthority()) return;
	UE_LOG(LogTemp, Display, TEXT("UATGContainerComponent::InitContainerItem"));

	for (auto& Item : ContainerItems)
	{
		if (Item.ItemDef.IsNull()) continue;

		if (!Item.ItemDef.IsValid())
		{
			Item.ItemDef.LoadSynchronous();
		}

		if (!Item.ItemDef.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("%s Failed to load Item: %s"), ANSI_TO_TCHAR(__FUNCTION__), *Item.ItemDef.ToString());
			continue;
		}

		UE_LOG(LogTemp, Display, TEXT("UATGContainerComponent::InitContainerItem Item: %s"), *Item.ItemDef->GetName());
		int32 W = Item.ItemDef->Width;
		int32 H = Item.ItemDef->Height;
		int32 OutX = -1, OutY = -1;
		int32 Qty = Item.Quantity;

		if (!ContainerInventory.FindFirstFit(Item.ItemDef, W, H, OutX, OutY, Qty))
		{
			continue;
		}

		if (Qty <= 0)
		{
			Item.Quantity = 0;
			continue;
		}

		ContainerInventory.AddItemAt(Item.ItemDef, Qty, OutX, OutY, W, H, false, -1);

		while (Qty >= 1)
		{
			OutX = -1;
			OutY = -1;
			if (!ContainerInventory.FindFirstFit(W, H, OutX, OutY))
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

void UATGContainerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UATGContainerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams RepParams;
	RepParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(UATGContainerComponent, GridSize, RepParams);

	DOREPLIFETIME_CONDITION(UATGContainerComponent, ContainerInventory, COND_None);
}

void UATGContainerComponent::OnRep_GridSize()
{
	ContainerInventory.GridWidth = GridSize.X;
	ContainerInventory.GridHeight = GridSize.Y;

	OnRebuildAll.Broadcast(-1);
}

void UATGContainerComponent::IncreaseGridSize(int32 W, int32 H)
{
	if (!(GetOwner() && GetOwner()->HasAuthority()))
	{
		return;
	}

	GridSize.X += W;
	GridSize.Y += H;

	MARK_PROPERTY_DIRTY_FROM_NAME(UATGContainerComponent, GridSize, this);

	OnRep_GridSize();
}

void UATGContainerComponent::DecreaseGridSize(int32 W, int32 H)
{
	if (!(GetOwner() && GetOwner()->HasAuthority()))
	{
		return;
	}

	GridSize.X -= W;
	GridSize.Y -= H;

	MARK_PROPERTY_DIRTY_FROM_NAME(UATGContainerComponent, GridSize, this);

	OnRep_GridSize();
}

void UATGContainerComponent::SetGridSize(int32 W, int32 H)
{
	if (!(GetOwner() && GetOwner()->HasAuthority()))
	{
		return;
	}

	GridSize.X = W;
	GridSize.Y = H;

	MARK_PROPERTY_DIRTY_FROM_NAME(UATGContainerComponent, GridSize, this);

	OnRep_GridSize();
}

FIntPoint UATGContainerComponent::GetGridSize()
{
	return FIntPoint(ContainerInventory.GridWidth, ContainerInventory.GridHeight);
}

void UATGContainerComponent::PlayerInteract(FInteractionData& InteractionData)
{
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

bool UATGContainerComponent::IsLocallyOwned()
{
	if (APlayerController* PC = GetOwner()->GetOwner() ? Cast<APlayerController>(GetOwner()->GetOwner()->GetOwner()) : nullptr)
	{
		return PC->IsLocalController();
	}
	return false;
}

bool UATGContainerComponent::CheckCanMove(int32 StartX, int32 StartY, int32 W, int32 H, int32 IgnoreId)
{
	return ContainerInventory.CheckMoveOrSwap(StartX, StartY, W, H, IgnoreId);
}

void UATGContainerComponent::TryMoveOrSwapClient(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
	UE_LOG(LogTemp, Display, TEXT("UATGContainerComponent::TryMoveOrSwapClient"));
	ServerMoveOrSwap(EntryId, NewX, NewY, bIsRotate);
}

void UATGContainerComponent::TrySplitStack(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate, int32 SplitNum)
{
	ServerSplitStack(EntryId, NewX, NewY, bIsRotate, SplitNum);
}

void UATGContainerComponent::TryDropItem(int32 EntryId, int32 SplitNum)
{
	ServerDropItem(EntryId, SplitNum);
}

void UATGContainerComponent::TryAddItemAt(TScriptInterface<IATGInventoryOwnerInterface> Inven, int32 OtherGridId, TSoftObjectPtr<class UATGItemData> ItemDef, int32 InQty, int32 X, int32 Y, bool bRotate)
{
	FClientAddRequest ClientAddRequest;
	ClientAddRequest.ItemDef = ItemDef;
	ClientAddRequest.Quantity = InQty;
	ClientAddRequest.X = X;
	ClientAddRequest.Y = Y;
	ClientAddRequest.bRotated = bRotate;
	ServerAddItemAt(ClientAddRequest, OtherGridId, Inven);
}

void UATGContainerComponent::TrySortByItemId()
{
	ServerSortByItemId();
}

void UATGContainerComponent::ServerSortByItemId_Implementation()
{
	ContainerInventory.SortEntryByItemId();
}

void UATGContainerComponent::ServerAddItemAt_Implementation(FClientAddRequest ClientAddRequest, int32 OtherGridId, const TScriptInterface<IATGInventoryOwnerInterface>& Inven)
{
	if (ClientAddRequest.ItemDef.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s ItemDef IsNull"), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	if (!ClientAddRequest.ItemDef.IsValid())
	{
		ClientAddRequest.ItemDef.LoadSynchronous();
	}

	if (!ClientAddRequest.ItemDef.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("%s Failed to load ItemDef"), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	int32 OriginQty = ClientAddRequest.Quantity;
	int32 Qty = ClientAddRequest.Quantity;

	ContainerInventory.AddItemAt(ClientAddRequest.ItemDef, Qty, ClientAddRequest.X, ClientAddRequest.Y, ClientAddRequest.ItemDef->Width, ClientAddRequest.ItemDef->Height, ClientAddRequest.bRotated);

	if (Qty > 0)
	{
		ClientAddRequest.Quantity = Qty;
		AddItemAuto(ClientAddRequest);
		Qty = ClientAddRequest.Quantity;
	}

	int32 DecreasedQty = OriginQty - Qty;
	if (Inven)
	{
		Inven->TryHandleTransItemResult(OtherGridId, DecreasedQty);
	}
}

TArray<int32> UATGContainerComponent::AddItemAuto(FClientAddRequest& ClientAddRequest)
{
	TArray<int32> EntryIds;
	EntryIds.Empty();
	if (ClientAddRequest.Quantity <= 0)
	{
		return EntryIds;
	}

	if (ClientAddRequest.ItemDef.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("%s ClientAddRequest.ItemDef.IsNull()"), ANSI_TO_TCHAR(__FUNCTION__));
		return EntryIds;
	}

	if (!ClientAddRequest.ItemDef.Get())
	{
		ClientAddRequest.ItemDef.LoadSynchronous();
	}

	if (!ClientAddRequest.ItemDef.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("%s Failed load ItemDef"), ANSI_TO_TCHAR(__FUNCTION__));
		return EntryIds;
	}

	int32 W = ClientAddRequest.ItemDef->Width;
	int32 H = ClientAddRequest.ItemDef->Height;
	int32 OutX = -1, OutY = -1;
	int32 RemainingQty = ClientAddRequest.Quantity;

	if (!ContainerInventory.FindFirstFit(ClientAddRequest.ItemDef, W, H, OutX, OutY, RemainingQty))
	{
		if (GetOwner()->HasAuthority())
		{
			ClientAddRequest.Quantity = RemainingQty;
		}
		return EntryIds;
	}

	if (RemainingQty <= 0)
	{
		if (GetOwner()->HasAuthority())
		{
			ClientAddRequest.Quantity = RemainingQty;
		}
		return EntryIds;
	}

	int32 Id = ContainerInventory.AddItemAt(ClientAddRequest.ItemDef, RemainingQty, OutX, OutY, W, H, false, ClientAddRequest.PredictionKey);
	EntryIds.Add(Id);

	while (RemainingQty >= 1)
	{
		OutX = -1;
		OutY = -1;
		if (!ContainerInventory.FindFirstFit(W, H, OutX, OutY))
		{
			break;
		}
		Id = ContainerInventory.AddItemAt(ClientAddRequest.ItemDef, RemainingQty, OutX, OutY, W, H, false, ClientAddRequest.PredictionKey);
		if (Id == 0)
		{
			break;
		}
		EntryIds.Add(Id);
	}

	if (GetOwner()->HasAuthority())
	{
		ClientAddRequest.Quantity = RemainingQty;
	}

	return EntryIds;
}

void UATGContainerComponent::ServerDropItem_Implementation(int32 EntryId, int32 SplitNum)
{
	ServerSpawnItem(EntryId, SplitNum);
	if (SplitNum > 0)
	{
		ContainerInventory.DecreaseQtyAndRemoveById(EntryId, SplitNum);
		return;
	}
	ServerRemoveItem(EntryId);
}

void UATGContainerComponent::ServerSpawnItem_Implementation(int32 EntryId, int32 SplitNum)
{
	if (!ItemBPClass)
	{
		return;
	}

	FInventoryEntry* Entry = ContainerInventory.GetById(EntryId);
	if (!Entry)
	{
		UE_LOG(LogTemp, Warning, TEXT("UATGContainerComponent::ServerSpawnItem Entry is Invalid"));
		return;
	}

	if (Entry->Item.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("UATGContainerComponent::ServerSpawnItem Entry Item is Null"));
		return;
	}

	if (!Entry->Item.IsValid())
	{
		if (!Entry->Item.LoadSynchronous())
		{
			UE_LOG(LogTemp, Warning, TEXT("UATGContainerComponent::ServerSpawnItem failed to load Entry Item"));
			return;
		}
	}

	AActor* SpawnOwner = nullptr;
	if (AActor* OwnerActor = GetOwner())
	{
		SpawnOwner = OwnerActor->GetOwner();
	}

	ACharacter* PlayerCharacter = Cast<ACharacter>(SpawnOwner);
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("UATGContainerComponent::ServerSpawnItem PlayerCharacter is invalid"));
		return;
	}

	const FVector SpawnLoc = PlayerCharacter->GetActorLocation() + FVector(100.f, 0.f, -50.f);
	const FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLoc, FVector(1.f));

	AATGItem* ItemActor = GetWorld()->SpawnActorDeferred<AATGItem>(
		ItemBPClass,
		SpawnTransform,
		PlayerCharacter,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn,
		ESpawnActorScaleMethod::OverrideRootScale);

	if (!ItemActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("UATGContainerComponent::ServerSpawnItem ItemActor is nullptr"));
		return;
	}

	UATGPickupComponent* PickupComp = ItemActor->GetPickupComp();
	if (!PickupComp)
	{
		ItemActor->Destroy();
		UE_LOG(LogTemp, Warning, TEXT("UATGContainerComponent::ServerSpawnItem PickupComp is nullptr"));
		return;
	}

	PickupComp->ItemDef = Entry->Item;
	PickupComp->ItemQty = SplitNum > 0 ? SplitNum : Entry->Quantity;
	UGameplayStatics::FinishSpawningActor(ItemActor, SpawnTransform);
	UE_LOG(LogTemp, Warning, TEXT("UATGContainerComponent::ServerSpawnItem spawned item"));
}

void UATGContainerComponent::ServerRemoveItem_Implementation(int32 EntryId)
{
	ContainerInventory.RemoveById(EntryId);
}

void UATGContainerComponent::TryHandleTransItemResult(int32 EntryId, int32 RemoveQty)
{
	ServerHandleTransItemResult(EntryId, RemoveQty);
}

void UATGContainerComponent::ServerHandleTransItemResult_Implementation(int32 EntryId, int32 RemoveQty)
{
	UE_LOG(LogTemp, Warning, TEXT("UATGContainerComponent::ServerHandleTransItemResult RemoveQty: %d"), RemoveQty);
	if (RemoveQty > 0)
	{
		ContainerInventory.DecreaseQtyAndRemoveById(EntryId, RemoveQty);
	}
}

void UATGContainerComponent::ServerSplitStack_Implementation(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate, int32 SplitNum)
{
	int32 Qty = SplitNum;
	FInventoryEntry* E = ContainerInventory.GetById(EntryId);

	if (!E)
	{
		UE_LOG(LogTemp, Warning, TEXT("UATGContainerComponent::ServerSplitStack Entry is Invalid"));
		return;
	}

	if (!E->Item.IsValid())
	{
		if (E->Item.LoadSynchronous())
		{
			UE_LOG(LogTemp, Warning, TEXT("UATGContainerComponent::ServerSplitStack E->Item.LoadSynchronous() Invalid"));
			return;
		}
	}

	if (ContainerInventory.AddItemAt(E->Item, Qty, NewX, NewY, E->Width, E->Height, bIsRotate, -1))
	{
		ContainerInventory.DecreaseQtyAndRemoveById(EntryId, SplitNum - Qty);
	}
	else
	{
		ContainerInventory.MergeStackAtAndDecrease(*E, SplitNum, NewX, NewY, bIsRotate);
	}
}

void UATGContainerComponent::ServerMoveOrSwap_Implementation(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
	bool S = ContainerInventory.MoveOrSwap(EntryId, NewX, NewY, bIsRotate);
	UE_LOG(LogTemp, Warning, TEXT("UATGContainerComponent::ServerMoveOrSwap %d"), S);
}

const FInventoryEntry& UATGContainerComponent::GetById(int32 EntryId)
{
	FInventoryEntry* Entry = ContainerInventory.GetById(EntryId);
	if (Entry)
	{
		return *Entry;
	}
	else
	{
		static FInventoryEntry EmptyEntry;
		return EmptyEntry;
	}
}
