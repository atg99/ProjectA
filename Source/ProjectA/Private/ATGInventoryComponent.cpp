// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "InventoryTypes.h"
#include "ATGItemData.h"
#include "GameFramework/PlayerState.h"

// Sets default values for this component's properties
UATGInventoryComponent::UATGInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	Inventory.OwnerComp = this;

	SetIsReplicatedByDefault(true);
	// ...
}


// Called when the game starts
void UATGInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	Inventory.OwnerComp = this;

	OnItemAdded.AddDynamic(this, &UATGInventoryComponent::HandleReplicatedAdd);
	//OnItemChanged.AddDynamic(this, &UATGInventoryComponent::HandleReplicatedChange);
	// ...
	
}


// Called every frame
void UATGInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UATGInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UATGInventoryComponent, Inventory, COND_OwnerOnly);
}

int32 UATGInventoryComponent::AddItemAuto(const FClientAddRequest& ClientAddRequest, FInventoryChangeResult& OutChangeResult)
{
    const bool bIsAuthority = IsHasAuthority();

    if (!ClientAddRequest.ItemDef || ClientAddRequest.Quantity <= 0)
    {
        if (bIsAuthority)
        {
            OutChangeResult.Status = EInventoryChangeStatus::Rejected;
            OutChangeResult.Reason = EInventoryRejectReason::InvalidItem;
            OutChangeResult.PredictionKey = ClientAddRequest.PredictionKey;
            OutChangeResult.RequestedQuantity = ClientAddRequest.Quantity;
            OutChangeResult.GrantedQuantity = 0;
        }
        return 0;
    }

    UATGItemData* ItemData = ClientAddRequest.ItemDef.Get();
    if (!ItemData)
    {
        ItemData = ClientAddRequest.ItemDef.LoadSynchronous();
    }
    if (!ItemData)
    {
        if (bIsAuthority)
        {
            OutChangeResult.Status = EInventoryChangeStatus::Rejected;
            OutChangeResult.Reason = EInventoryRejectReason::InvalidItem;
            OutChangeResult.PredictionKey = ClientAddRequest.PredictionKey;
            OutChangeResult.RequestedQuantity = ClientAddRequest.Quantity;
            OutChangeResult.GrantedQuantity = 0;
        }
        return 0;
    }

    const int32 MaxStack = FMath::Max(ItemData->MaxStack, 1);
    const int32 Width = ItemData->Width;
    const int32 Height = ItemData->Height;

    int32 RemainingQty = ClientAddRequest.Quantity;
    int32 TotalGranted = 0;
    int32 FirstMergedEntryId = -1;
    int32 FirstMergedPrevQuantity = 0;
    int32 LastEntryId = 0;

    if (bIsAuthority)
    {
        OutChangeResult.Status = EInventoryChangeStatus::Rejected;
        OutChangeResult.Reason = EInventoryRejectReason::Unknown;
        OutChangeResult.PredictionKey = ClientAddRequest.PredictionKey;
        OutChangeResult.InventoryRevision = 0;
        OutChangeResult.ItemDefId = ItemData->GetPrimaryAssetId();
        OutChangeResult.NewEntryId = -1;
        OutChangeResult.MergedIntoEntryId = -1;
        OutChangeResult.PreviousQuantity = 0;
        OutChangeResult.X = -1;
        OutChangeResult.Y = -1;
        OutChangeResult.W = Width;
        OutChangeResult.H = Height;
        OutChangeResult.bRotated = ClientAddRequest.bRotated;
        OutChangeResult.RequestedQuantity = ClientAddRequest.Quantity;
        OutChangeResult.GrantedQuantity = 0;
        OutChangeResult.bServerAutoPlaced = false;
        OutChangeResult.SuggestedX = -1;
        OutChangeResult.SuggestedY = -1;
    }

    
    for (FInventoryEntry& Entry : Inventory.Entries)
    {
        if (RemainingQty <= 0)
        {
            break;
        }

        if (Entry.Item != ClientAddRequest.ItemDef)
        {
            continue;
        }

        const int32 Space = MaxStack - Entry.Quantity;
        if (Space <= 0)
        {
            continue;
        }

        const int32 Added = FMath::Min(Space, RemainingQty);
        if (Added <= 0)
        {
            continue;
        }

        if (FirstMergedEntryId == -1)
        {
            FirstMergedEntryId = Entry.Id;
            FirstMergedPrevQuantity = Entry.Quantity;
        }

        if (bIsAuthority)
        {
            Entry.Quantity += Added;
            Inventory.MarkItemDirty(Entry);
            OnItemChanged.Broadcast(Entry.Id);
        }

        RemainingQty -= Added;
        TotalGranted += Added;
    }

    if (bIsAuthority)
    {
        OutChangeResult.MergedIntoEntryId = FirstMergedEntryId;
        OutChangeResult.PreviousQuantity = FirstMergedPrevQuantity;
    }

    if (RemainingQty <= 0)
    {
        if (bIsAuthority)
        {
            OutChangeResult.Status = EInventoryChangeStatus::Success;
            OutChangeResult.Reason = EInventoryRejectReason::None;
            OutChangeResult.GrantedQuantity = TotalGranted;
        }
        return (FirstMergedEntryId > 0) ? FirstMergedEntryId : 0;
    }

    while (RemainingQty > 0)
    {
        int32 OutX = -1;
        int32 OutY = -1;

        if (!Inventory.FindFirstFit(Width, Height, OutX, OutY))
        {
            break;
        }

        const int32 StackQty = FMath::Min(RemainingQty, MaxStack);
        const int32 NewId = Inventory.AddItemAt(ClientAddRequest.ItemDef, StackQty, OutX, OutY, Width, Height, false, ClientAddRequest.PredictionKey);
        if (NewId <= 0)
        {
            break;
        }

        RemainingQty -= StackQty;
        TotalGranted += StackQty;
        LastEntryId = NewId;

        if (bIsAuthority && OutChangeResult.NewEntryId == -1)
        {
            OutChangeResult.NewEntryId = NewId;
            OutChangeResult.X = OutX;
            OutChangeResult.Y = OutY;
            OutChangeResult.W = Width;
            OutChangeResult.H = Height;
            OutChangeResult.bRotated = false;
            OutChangeResult.bServerAutoPlaced = true;
        }

        if (!bIsAuthority)
        {
            break;
        }
    }

    if (bIsAuthority)
    {
        OutChangeResult.GrantedQuantity = TotalGranted;
        if (RemainingQty > 0)
        {
            OutChangeResult.Status = (TotalGranted > 0) ? EInventoryChangeStatus::PartialSuccess : EInventoryChangeStatus::Rejected;
            OutChangeResult.Reason = EInventoryRejectReason::NoSpace;
        }
        else
        {
            OutChangeResult.Status = EInventoryChangeStatus::Success;
            OutChangeResult.Reason = EInventoryRejectReason::None;
        }
    }

    if (TotalGranted <= 0)
    {
        return 0;
    }

    if (LastEntryId > 0)
    {
        return LastEntryId;
    }

    return (FirstMergedEntryId > 0) ? FirstMergedEntryId : 0;
}

void UATGInventoryComponent::TryPickupClient(TSoftObjectPtr<UATGItemData> ItemDef, int32 Quantity)
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
    FInventoryChangeResult R; //클라에서는 사용X
	int32 Id = AddItemAuto(ClientAddRequest, R);

	if (Id == 0) //클라에서 판정실패시 서버요청 안함
	{
		return;
	}

	ServerAddItemAuto(ClientAddRequest);
}

void UATGInventoryComponent::ServerAddItemAuto_Implementation(FClientAddRequest ClientAddRequest)
{
	FInventoryChangeResult InventoryChangeResult;

    AddItemAuto(ClientAddRequest, InventoryChangeResult);
	
	ClientAddItemResult(InventoryChangeResult);
}

void UATGInventoryComponent::ClientAddItemResult_Implementation(FInventoryChangeResult Result)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Emerald, TEXT("ClientCallBackAddItem"));
	if (Result.Status == EInventoryChangeStatus::Success)
	{
		// 성공이면 복제 도착 시점(HandleReplicatedAdd)에서 지우므로 생략
	}
	else
	{
		// 실패 → 프리뷰 즉시 제거
		if (Result.PredictionKey != 0)
		{
			Inventory.PreviewRemoveById(Result.PredictionKey);
			OnItemPreRemoved.Broadcast(Result.PredictionKey);
		}
	}
}

void UATGInventoryComponent::ServerAddItemAt_Implementation(UATGItemData* ItemDef, int32 Quantity, int32 X, int32 Y, bool bRotated)
{
	if (!ItemDef || Quantity <= 0) return;

	int32 W = bRotated ? ItemDef->Height : ItemDef->Width;
	int32 H = bRotated ? ItemDef->Width : ItemDef->Height;

	const int32 Id = Inventory.AddItemAt(ItemDef, Quantity, X, Y, W, H, bRotated, 0);
	//if (Id > 0) OnItemAdded.Broadcast(Id);
}

void UATGInventoryComponent::TryMoveOrSwapClient(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
	if (!IsLocallyOwned())
	{
		return;
	}

	if (!Inventory.PreviewMoveOrSwap(EntryId, NewX, NewY, bIsRotate))
	{
		return; // 놓을 수 없으면 서버 호출 안함
	}
	
	ServerMoveOrSwap(EntryId, NewX, NewY, bIsRotate);
}

void UATGInventoryComponent::ServerMoveOrSwap_Implementation(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
	bool bIsSuccessful = Inventory.MoveOrSwap(EntryId, NewX, NewY, bIsRotate);
	FInventoryChangeResult Result;
	
	Result.Status = (bIsSuccessful ? EInventoryChangeStatus::Success : EInventoryChangeStatus::Rejected);
	Result.NewEntryId = EntryId;
	ClientMoveResult(Result);
}

void UATGInventoryComponent::ClientMoveResult_Implementation(const FInventoryChangeResult& Result)
{
	if (Result.Status == EInventoryChangeStatus::Rejected)
	{
		Inventory.PreviewRemoveById(Result.NewEntryId);
		OnItemPreRemoved.Broadcast(Result.NewEntryId);
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("!!! ClientMoveResult : Rejected"));
	}
}

void UATGInventoryComponent::ServerRotateItem_Implementation(int32 EntryId)
{
	Inventory.Rotate(EntryId);
		//OnItemChanged.Broadcast(EntryId);
}

void UATGInventoryComponent::ServerRemoveItem_Implementation(int32 EntryId)
{
	Inventory.RemoveById(EntryId);
		//OnItemRemoved.Broadcast(EntryId);
}

void UATGInventoryComponent::HandleReplicatedAdd(int32 EntryId)
{
	const FInventoryEntry* E = Inventory.GetById(EntryId);
	if (E && E->PredictionKey != 0)
	{
		Inventory.PreviewRemoveById(E->PredictionKey); // PredKey로 직접 제거
		OnItemPreRemoved.Broadcast(E->PredictionKey);  // 위젯에게도 알려 제거
	}
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("!!! InventComp HandleReplicatedAddp"));
}

//void UATGInventoryComponent::HandleReplicatedChange(int32 EntryId)
//{
//	Inventory.GetById(EntryId);
//}

bool UATGInventoryComponent::IsHasAuthority()
{
	if (const APlayerState* PS = Cast<APlayerState>(GetOwner()))
	{
		if (const APlayerController* PC = Cast<APlayerController>(PS->GetOwner()))
		{
			return PC->HasAuthority();
		}
	}
	return false;
}

bool UATGInventoryComponent::IsLocallyOwned() const
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