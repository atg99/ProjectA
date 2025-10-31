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

int32 UATGInventoryComponent::AddItemAuto(const FClientAddRequest& ClientAddRequest)
{
	if (!ClientAddRequest.ItemDef || ClientAddRequest.Quantity <= 0) return -1;

	int32 W = ClientAddRequest.ItemDef->Width;
	int32 H = ClientAddRequest.ItemDef->Height;
	int32 OutX = -1, OutY = -1;

	if (!Inventory.FindFirstFit(W, H, OutX, OutY))
		return -1; // ���� á��

	const int32 Id = Inventory.AddItemAt(ClientAddRequest.ItemDef, ClientAddRequest.Quantity, OutX, OutY, W, H, false, ClientAddRequest.PredictionKey);
	return Id;
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

	int32 Id = AddItemAuto(ClientAddRequest);

	if (Id == 0) //Ŭ�󿡼� �������н� ������û ����
	{
		return;
	}

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
	
	ClientAddItemResult(InventoryChangeResult);
	
	//if (Id > 0) OnItemAdded.Broadcast(Id);
}

void UATGInventoryComponent::ClientAddItemResult_Implementation(FInventoryChangeResult Result)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Emerald, TEXT("ClientCallBackAddItem"));
	if (Result.Status == EInventoryChangeStatus::Success)
	{
		// �����̸� ���� ���� ����(HandleReplicatedAdd)���� ����Ƿ� ����
	}
	else
	{
		// ���� �� ������ ��� ����
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
		return; // ���� �� ������ ���� ȣ�� ����
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
		Inventory.PreviewRemoveById(E->PredictionKey); // PredKey�� ���� ����
		OnItemPreRemoved.Broadcast(E->PredictionKey);  // �������Ե� �˷� ����
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