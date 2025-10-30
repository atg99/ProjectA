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

int32 UATGInventoryComponent::AddItemAuto(const FClientAddRequest& ClientAddRequest)
{
	if (!ClientAddRequest.ItemDef || ClientAddRequest.Quantity <= 0) return -1;

	int32 W = ClientAddRequest.ItemDef->Width;
	int32 H = ClientAddRequest.ItemDef->Height;
	int32 OutX = -1, OutY = -1;

	if (!Inventory.FindFirstFit(W, H, OutX, OutY))
		return -1; // ���� á��

	const int32 Id = Inventory.AddItemAt(ClientAddRequest.ItemDef, ClientAddRequest.Quantity, OutX, OutY, W, H, false);
	return Id;
}

void UATGInventoryComponent::TryPickupClient(TSoftObjectPtr<UATGItemData> ItemDef, int32 Quantity)
{
	if (!IsLocallyOwned())
	{
		return;
	}

	FClientAddRequest ClientAddRequest;

	ClientAddRequest.ItemDef = ItemDef;
	ClientAddRequest.Quantity = Quantity;
	ClientAddRequest.X = -1;
	ClientAddRequest.Y = -1;
	ClientAddRequest.PredictionKey = -1;

	int32 Id = AddItemAuto(ClientAddRequest);

	if (Id < 0) //Ŭ�󿡼� �������н� ������û ����
	{
		return;
	}

	ClientAddRequest.PredictionKey = Id;

	ServerAddItemAuto(ClientAddRequest);
}

void UATGInventoryComponent::ServerAddItemAuto_Implementation(FClientAddRequest ClientAddRequest)
{
	FInventoryChangeResult InventoryChangeResult;
	const int32 Id = AddItemAuto(ClientAddRequest);
	if (Id >= 0)
	{
		InventoryChangeResult.Status = EInventoryChangeStatus::Success;
	}
	else
	{
		InventoryChangeResult.Status = EInventoryChangeStatus::Rejected;
		InventoryChangeResult.Reason = EInventoryRejectReason::Unknown;
	}

	InventoryChangeResult.PredictionKey = ClientAddRequest.PredictionKey; //���� Ŭ���̾�Ʈ ��Ī Ű

	InventoryChangeResult.NewEntryId = Id;
	
	ClientCallBackAddItem(InventoryChangeResult);
	
	//if (Id > 0) OnItemAdded.Broadcast(Id);
}

void UATGInventoryComponent::ClientCallBackAddItem_Implementation(FInventoryChangeResult Result)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Emerald, TEXT("ClientCallBackAddItem"));
	if (Result.Status == EInventoryChangeStatus::Success)
	{
		// ���� �� OnRep���� �����並 ������ �ϹǷ� ���θ� �߰�
		if (Result.NewEntryId > 0 && Result.PredictionKey > 0)
		{
			PendingServerToPred.Add(Result.NewEntryId, Result.PredictionKey);
		}
	}
	else
	{
		// ���� �� �κ��丮�� �׸��� �� ���Ƿ� ������ ��� ����
		if (Result.PredictionKey > 0)
		{
			Inventory.PreviewRemoveById(Result.PredictionKey);
		}
	}
}

void UATGInventoryComponent::ServerAddItemAt_Implementation(UATGItemData* ItemDef, int32 Quantity, int32 X, int32 Y, bool bRotated)
{
	if (!ItemDef || Quantity <= 0) return;

	int32 W = bRotated ? ItemDef->Height : ItemDef->Width;
	int32 H = bRotated ? ItemDef->Width : ItemDef->Height;

	const int32 Id = Inventory.AddItemAt(ItemDef, Quantity, X, Y, W, H, bRotated);
	//if (Id > 0) OnItemAdded.Broadcast(Id);
}

void UATGInventoryComponent::ServerMoveOrSwap_Implementation(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
	Inventory.MoveOrSwap(EntryId, NewX, NewY, bIsRotate);
		//OnItemChanged.Broadcast(EntryId);
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

void UATGInventoryComponent::HandleReplicatedAdd(int32 ServerEntryId)
{
	if (int32* Pred = PendingServerToPred.Find(ServerEntryId))
	{
		Inventory.PreviewRemoveById(*Pred);   // �� PredictionId�� ������ ����
		PendingServerToPred.Remove(ServerEntryId);
	}
}

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