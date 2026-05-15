// Fill out your copyright notice in the Description page of Project Settings.

#include "ATGInventoryComponent.h"
#include "ATGItem.h"
#include "ATGPickupComponent.h"
#include "Net/UnrealNetwork.h"
#include "InventoryTypes.h"
#include "Data/ATGItemData.h"
#include "Data/ATGConsumableItemData.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Blueprint/UserWidget.h"
#include "Widget/ATGItemContextMenuWidget.h"
#include "AbilitySystemBlueprintLibrary.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Item_Use, "Event.Item.Use", "event when use consumableitem");

UATGInventoryComponent::UATGInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	Inventory.Owner = TScriptInterface<IATGInventoryOwnerInterface>(this);
	SetIsReplicatedByDefault(true);
}

void UATGInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	Inventory.Owner = TScriptInterface<IATGInventoryOwnerInterface>(this);

	OnItemAdded.AddDynamic(this, &UATGInventoryComponent::HandleReplicatedAdd);

	if (IsLocallyOwned())
	{
		if (ContextMenuClass)
		{
			if (APlayerState* PS = Cast<APlayerState>(GetOwner()))
			{
				ContextMenuWidget = CreateWidget<UATGItemContextMenuWidget>(PS->GetPlayerController(), ContextMenuClass);
				if (ContextMenuWidget)
				{
					ContextMenuWidget->InvenComp = this;
					ContextMenuWidget->AddToViewport(100);
					ContextMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
	}
}

void UATGInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UATGInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams RepParams;
	RepParams.bIsPushBased = true;
	RepParams.Condition = ELifetimeCondition::COND_OwnerOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(UATGInventoryComponent, GridSize, RepParams);

	DOREPLIFETIME_CONDITION(UATGInventoryComponent, Inventory, COND_OwnerOnly);
}

void UATGInventoryComponent::ItemAdded(int32 EntryId)
{
	UE_LOG(LogTemp, Log, TEXT("UATGInventoryComponent::ItemAdded %d"), EntryId);
	OnItemAdded.Broadcast(EntryId);
}

void UATGInventoryComponent::ItemChanged(int32 EntryId)
{
	UE_LOG(LogTemp, Log, TEXT("UATGInventoryComponent::ItemChanged %d"), EntryId);
	OnItemChanged.Broadcast(EntryId);
}

void UATGInventoryComponent::InventoryForceNetUpdate()
{
	GetOwner()->ForceNetUpdate();
}

void UATGInventoryComponent::OnRep_GridSize()
{
	Inventory.GridWidth = GridSize.X;
	Inventory.GridHeight = GridSize.Y;

	OnRebuildAll.Broadcast(-1);
}

void UATGInventoryComponent::ItemRemoved(int32 EntryId)
{
	UE_LOG(LogTemp, Log, TEXT("UATGInventoryComponent::ItemRemoved %d"), EntryId);
	OnItemRemoved.Broadcast(EntryId);
}

TArray<int32> UATGInventoryComponent::AddItemAuto(FClientAddRequest& ClientAddRequest, AActor* InteractedActor)
{
	UE_LOG(LogTemp, Log, TEXT("UATGInventoryComponent::AddItemAuto"));
	TArray<int32> EntryIds;
	EntryIds.Empty();
	if (!ClientAddRequest.ItemDef.Get())
	{
		if (!ClientAddRequest.ItemDef.LoadSynchronous())
		{
			UE_LOG(LogTemp, Warning, TEXT("UATGInventoryComponent::AddItemAuto !ClientAddRequest.ItemDef.LoadSynchronous() %s"), *ClientAddRequest.ItemDef.GetAssetName());
			return EntryIds;
		}
	}

	if (!ClientAddRequest.ItemDef || ClientAddRequest.Quantity <= 0)
	{
		return EntryIds;
	}

	int32 W = ClientAddRequest.ItemDef->Width;
	int32 H = ClientAddRequest.ItemDef->Height;
	int32 OutX = -1, OutY = -1;
	int32 OriginQty = ClientAddRequest.Quantity;
	int32 Qty = ClientAddRequest.Quantity;

	if (!Inventory.FindFirstFit(ClientAddRequest.ItemDef, W, H, OutX, OutY, Qty))
	{
		if (IsHasAuthority())
		{
			if (UATGPickupComponent* Comp = GetPickupComp(InteractedActor))
			{
				Comp->DecreaseQty(OriginQty - Qty);
			}
			ClientAddRequest.Quantity = Qty;
		}
		UE_LOG(LogTemp, Log, TEXT("UATGInventoryComponent::AddItemAuto !FindFirstFit"));
		return EntryIds;
	}

	if (Qty <= 0)
	{
		if (IsHasAuthority())
		{
			if (UATGPickupComponent* Comp = GetPickupComp(InteractedActor))
			{
				Comp->DecreaseQty(OriginQty - Qty);
			}
			ClientAddRequest.Quantity = Qty;
		}
		return EntryIds;
	}

	int32 Id = Inventory.AddItemAt(ClientAddRequest.ItemDef, Qty, OutX, OutY, W, H, false, ClientAddRequest.PredictionKey);
	EntryIds.Add(Id);

	while (Qty >= 1)
	{
		OutX = -1;
		OutY = -1;
		if (!Inventory.FindFirstFit(W, H, OutX, OutY))
		{
			break;
		}
		Id = Inventory.AddItemAt(ClientAddRequest.ItemDef, Qty, OutX, OutY, W, H, false, ClientAddRequest.PredictionKey);
		if (Id == 0)
		{
			break;
		}
		EntryIds.Add(Id);
	}

	if (IsHasAuthority())
	{
		if (UATGPickupComponent* Comp = GetPickupComp(InteractedActor))
		{
			Comp->DecreaseQty(OriginQty - Qty);
		}
		ClientAddRequest.Quantity = Qty;
	}

	return EntryIds;
}

void UATGInventoryComponent::ServerAddItemAt_Implementation(FClientAddRequest ClientAddRequest, int32 OtherGridId, const TScriptInterface<IATGInventoryOwnerInterface>& Inven)
{
	UE_LOG(LogTemp, Log, TEXT("UATGInventoryComponent::ServerAddItemAt"));
	int32 OriginQty = ClientAddRequest.Quantity;
	int32 Qty = ClientAddRequest.Quantity;

	if (ClientAddRequest.ItemDef.IsNull())
	{
		return;
	}

	if (!ClientAddRequest.ItemDef.IsValid())
	{
		if (!ClientAddRequest.ItemDef.LoadSynchronous())
		{
			return;
		}
	}

	Inventory.AddItemAt(ClientAddRequest.ItemDef, Qty, ClientAddRequest.X, ClientAddRequest.Y, ClientAddRequest.ItemDef->Width, ClientAddRequest.ItemDef->Height, ClientAddRequest.bRotated);

	if (Qty > 0)
	{
		ClientAddRequest.Quantity = Qty;
		AddItemAuto(ClientAddRequest, nullptr);
		Qty = ClientAddRequest.Quantity;
	}

	int32 DecreasedQty = OriginQty - Qty;
	if (Inven)
	{
		Inven->TryHandleTransItemResult(OtherGridId, DecreasedQty);
	}
}

void UATGInventoryComponent::TryAddItemAt(TScriptInterface<IATGInventoryOwnerInterface> Inven, int32 OtherGridId, TSoftObjectPtr<UATGItemData> ItemDef, int32 InQty, int32 X, int32 Y, bool bRotate)
{
	FClientAddRequest ClientAddRequest;
	ClientAddRequest.ItemDef = ItemDef;
	ClientAddRequest.Quantity = InQty;
	ClientAddRequest.X = X;
	ClientAddRequest.Y = Y;
	ClientAddRequest.bRotated = bRotate;
	ServerAddItemAt(ClientAddRequest, OtherGridId, Inven);
}

void UATGInventoryComponent::TryPickupClient(TSoftObjectPtr<UATGItemData> ItemDef, int32 Quantity, AActor* InteractActor)
{
	if (!IsLocallyOwned())
	{
		return;
	}

	int32 PredKey = LocalPred--;

	FClientAddRequest ClientAddRequest;
	ClientAddRequest.ItemDef = ItemDef;
	ClientAddRequest.Quantity = Quantity;
	ClientAddRequest.X = -1;
	ClientAddRequest.Y = -1;
	ClientAddRequest.PredictionKey = PredKey;

	TArray<int32> EntryIds = AddItemAuto(ClientAddRequest, InteractActor);

	if (!IsHasAuthority())
	{
		ServerAddItemAuto(ClientAddRequest, InteractActor);
	}
}

void UATGInventoryComponent::ServerAddItemAuto_Implementation(FClientAddRequest ClientAddRequest, AActor* InteractedActor)
{
	FInventoryChangeResult InventoryChangeResult;
	TArray<int32> EntryIds = AddItemAuto(ClientAddRequest, InteractedActor);
	if (!EntryIds.IsEmpty())
	{
		InventoryChangeResult.Status = EInventoryChangeStatus::Success;
	}
	else
	{
		InventoryChangeResult.Status = EInventoryChangeStatus::Rejected;
		InventoryChangeResult.Reason = EInventoryRejectReason::Unknown;
	}

	InventoryChangeResult.PredictionKey = ClientAddRequest.PredictionKey;
	InventoryChangeResult.NewEntryIds = EntryIds;

	ClientAddItemResult(InventoryChangeResult);
}

void UATGInventoryComponent::ClientAddItemResult_Implementation(FInventoryChangeResult Result)
{
}

void UATGInventoryComponent::TryMoveOrSwapClient(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
	if (!IsLocallyOwned())
	{
		return;
	}
	ServerMoveOrSwap(EntryId, NewX, NewY, bIsRotate);
}

bool UATGInventoryComponent::CheckCanMove(int32 StartX, int32 StartY, int32 W, int32 H, int32 Id)
{
	return Inventory.CheckMoveOrSwap(StartX, StartY, W, H, Id);
}

void UATGInventoryComponent::TrySplitStack(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate, int32 SplitNum)
{
	ServerSplitStack(EntryId, NewX, NewY, bIsRotate, SplitNum);
}

void UATGInventoryComponent::ServerSplitStack_Implementation(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate, int32 SplitNum)
{
	int32 Qty = SplitNum;
	FInventoryEntry* E = Inventory.GetById(EntryId);
	if (!E)
	{
		UE_LOG(LogTemp, Warning, TEXT("UATGInventoryComponent::ServerSplitStack E is Invalid"));
		return;
	}

	if (Inventory.AddItemAt(E->Item, Qty, NewX, NewY, E->Width, E->Height, bIsRotate, -1))
	{
		Inventory.DecreaseQtyAndRemoveById(EntryId, SplitNum - Qty);
	}
	else
	{
		Inventory.MergeStackAtAndDecrease(*E, SplitNum, NewX, NewY, bIsRotate);
	}
}

void UATGInventoryComponent::TryDropItem(int32 EntryId, int32 SplitNum)
{
	ServerDropItem(EntryId, SplitNum);
}

void UATGInventoryComponent::TryHandleTransItemResult(int32 EntryId, int32 RemoveQty)
{
	ServerHandleTransItemResult(EntryId, RemoveQty);
}

void UATGInventoryComponent::ServerHandleTransItemResult_Implementation(int32 EntryId, int32 RemoveQty)
{
	if (RemoveQty > 0)
	{
		Inventory.DecreaseQtyAndRemoveById(EntryId, RemoveQty);
	}
}

void UATGInventoryComponent::TrySortByItemId()
{
	ServerSortByItemId();
}

void UATGInventoryComponent::ServerSortByItemId_Implementation()
{
	Inventory.SortEntryByItemId();
}

void UATGInventoryComponent::ServerDropItem_Implementation(int32 EntryId, int32 SplitNum)
{
	if (!ItemBPClass)
	{
		return;
	}

	ServerSpawnItem(EntryId, SplitNum);
	if (SplitNum > 0)
	{
		Inventory.DecreaseQtyAndRemoveById(EntryId, SplitNum);
		return;
	}
	ServerRemoveItem(EntryId);
}

void UATGInventoryComponent::ServerMoveOrSwap_Implementation(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
	UE_LOG(LogTemp, Warning, TEXT("UATGInventoryComponent::ServerMoveOrSwap %d"), bIsRotate);
	bool bIsSuccessful = Inventory.MoveOrSwap(EntryId, NewX, NewY, bIsRotate);
	FInventoryChangeResult Result;

	Result.Status = (bIsSuccessful ? EInventoryChangeStatus::Success : EInventoryChangeStatus::Rejected);
	Result.NewEntryIds.Add(EntryId);
	ClientMoveResult(Result);
}

void UATGInventoryComponent::ClientMoveResult_Implementation(const FInventoryChangeResult& Result)
{
}

void UATGInventoryComponent::ServerRotateItem_Implementation(int32 EntryId)
{
	Inventory.Rotate(EntryId);
}

void UATGInventoryComponent::ServerRemoveItem_Implementation(int32 EntryId)
{
	Inventory.RemoveById(EntryId);
}

void UATGInventoryComponent::ServerDecreaseItem_Implementation(int32 EntryId, int32 Qty)
{
	Inventory.DecreaseQtyAndRemoveById(EntryId, Qty);
}

void UATGInventoryComponent::ServerSpawnItem_Implementation(int32 EntryId, int32 SplitNum)
{
	if (!ItemBPClass)
	{
		return;
	}

	FInventoryEntry* Entry = Inventory.GetById(EntryId);
	if (!Entry)
	{
		UE_LOG(LogTemp, Warning, TEXT("UATGInventoryComponent::ServerSpawnItem Entry is Invalid"));
		return;
	}

	if (Entry->Item.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("UATGInventoryComponent::ServerSpawnItem Entry Item is Null"));
		return;
	}

	if (!Entry->Item.IsValid())
	{
		if (!Entry->Item.LoadSynchronous())
		{
			UE_LOG(LogTemp, Warning, TEXT("UATGInventoryComponent::ServerSpawnItem failed to load Entry Item"));
			return;
		}
	}

	APlayerState* PS = Cast<APlayerState>(GetOwner());
	APawn* Pawn = PS ? PS->GetPawn() : nullptr;
	if (!Pawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("UATGInventoryComponent::ServerSpawnItem Pawn is invalid"));
		return;
	}

	const FVector SpawnLoc = Pawn->GetActorLocation() + FVector(100.f, 0.f, -50.f);
	const FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLoc, FVector(1.f));

	AATGItem* ItemActor = GetWorld()->SpawnActorDeferred<AATGItem>(
		ItemBPClass,
		SpawnTransform,
		Pawn,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn,
		ESpawnActorScaleMethod::OverrideRootScale);

	if (!ItemActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("UATGInventoryComponent::ServerSpawnItem ItemActor is nullptr"));
		return;
	}

	UATGPickupComponent* PickupComp = ItemActor->GetPickupComp();
	if (!PickupComp)
	{
		ItemActor->Destroy();
		UE_LOG(LogTemp, Warning, TEXT("UATGInventoryComponent::ServerSpawnItem PickupComp is nullptr"));
		return;
	}

	PickupComp->ItemDef = Entry->Item;
	PickupComp->ItemQty = SplitNum > 0 ? SplitNum : Entry->Quantity;
	UGameplayStatics::FinishSpawningActor(ItemActor, SpawnTransform);
	UE_LOG(LogTemp, Warning, TEXT("UATGInventoryComponent::ServerSpawnItem spawned item"));
}

void UATGInventoryComponent::HandleReplicatedAdd(int32 EntryId)
{
}

void UATGInventoryComponent::UseItem(const FATGItemInfo& ItemInfo)
{
	APlayerState* PS = Cast<APlayerState>(GetOwner());
	APawn* AvatarPawn = PS ? PS->GetPawn() : nullptr;
	if (!AvatarPawn)
	{
		return;
	}

	UATGItemData* ItemData = ItemInfo.ItemDef.Get();
	if (!ItemData)
	{
		ItemData = ItemInfo.ItemDef.LoadSynchronous();
	}

	UATGConsumableItemData* ConsumableItemData = Cast<UATGConsumableItemData>(ItemData);
	if (!ConsumableItemData || !ConsumableItemData->AbilityTriggerTag.IsValid())
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventMagnitude = ItemInfo.EntryId;
	Payload.Instigator = AvatarPawn;
	Payload.Target = AvatarPawn;
	Payload.EventTag = ConsumableItemData->AbilityTriggerTag;
	Payload.OptionalObject = ConsumableItemData;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(AvatarPawn, Payload.EventTag, Payload);
}

void UATGInventoryComponent::OpenContextMenu(FATGItemInfo& ItemInfo, FVector2D ScreenPosition)
{
	if (!ContextMenuWidget)
	{
		return;
	}

	ContextMenuWidget->InitMenu(ItemInfo);
	ContextMenuWidget->SetPositionInViewport(ScreenPosition, true);
	ContextMenuWidget->SetVisibility(ESlateVisibility::Visible);
}

bool UATGInventoryComponent::IsHasAuthority()
{
	return (GetOwner() && GetOwner()->HasAuthority());
}

bool UATGInventoryComponent::IsLocallyOwned()
{
	if (const APlayerState* PS = Cast<APlayerState>(GetOwner()))
	{
		if (const APlayerController* PC = Cast<APlayerController>(PS->GetOwner()))
			return PC->IsLocalController();
	}
	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
		return Pawn->IsLocallyControlled();

	if (const APlayerController* PC = Cast<APlayerController>(GetOwner()))
		return PC->IsLocalController();

	return false;
}

void UATGInventoryComponent::IncreaseGridSize(int32 W, int32 H)
{
	if (!IsHasAuthority())
	{
		return;
	}

	GridSize.X += W;
	GridSize.Y += H;

	MARK_PROPERTY_DIRTY_FROM_NAME(UATGInventoryComponent, GridSize, this);

	OnRep_GridSize();
}

void UATGInventoryComponent::DecreaseGridSize(int32 W, int32 H)
{
	if (!IsHasAuthority())
	{
		return;
	}

	GridSize.X -= W;
	GridSize.Y -= H;

	MARK_PROPERTY_DIRTY_FROM_NAME(UATGInventoryComponent, GridSize, this);

	OnRep_GridSize();
}

void UATGInventoryComponent::SetGridSize(int32 W, int32 H)
{
	if (!IsHasAuthority())
	{
		return;
	}

	GridSize.X = W;
	GridSize.Y = H;

	MARK_PROPERTY_DIRTY_FROM_NAME(UATGInventoryComponent, GridSize, this);

	OnRep_GridSize();
}

FIntPoint UATGInventoryComponent::GetGridSize()
{
	return GridSize;
}

UATGPickupComponent* UATGInventoryComponent::GetPickupComp(AActor* InteractedActor)
{
	if (InteractedActor)
	{
		return InteractedActor->GetComponentByClass<UATGPickupComponent>();
	}

	return nullptr;
}
