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

TArray<int32> UATGInventoryComponent::AddItemAuto(const FClientAddRequest& ClientAddRequest)
{
	TArray<int32> EntryIds;
	EntryIds.Empty();
	if (!ClientAddRequest.ItemDef || ClientAddRequest.Quantity <= 0) return EntryIds;

	if (!ClientAddRequest.ItemDef.Get()) // �ε�
	{
		ClientAddRequest.ItemDef.LoadSynchronous();
	}

	int32 W = ClientAddRequest.ItemDef->Width;
	int32 H = ClientAddRequest.ItemDef->Height;
	int32 OutX = -1, OutY = -1;
	int32 Qty = ClientAddRequest.Quantity;

	if (!Inventory.FindFirstFit(ClientAddRequest.ItemDef, W, H, OutX, OutY, Qty)) //���⼭ �����ϴ� ���ÿ� ���� ���� �� Qty ������ ��ȯ
	{
		return EntryIds; // ���ο� �ڸ� ���� 
	}

	if (Qty <= 0) //������ 0�� �Ȱ�� 
	{
		return EntryIds;
	}

	int32 Id = Inventory.AddItemAt(ClientAddRequest.ItemDef, Qty, OutX, OutY, W, H, false, ClientAddRequest.PredictionKey);
	EntryIds.Add(Id);
	//Qty ���� ��ȯ
	while (Qty >= 1) //������ 0�� �ɶ� ���� �ݺ�
	{
		Inventory.FindFirstFit(W, H, OutX, OutY); //�ٽ� �ڸ� �˻�, �����ϴ� ���� ���� X 
		Id = Inventory.AddItemAt(ClientAddRequest.ItemDef, Qty, OutX, OutY, W, H, false, ClientAddRequest.PredictionKey);
		EntryIds.Add(Id);
	}
	
	return EntryIds;
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

	TArray<int32> EntryIds = AddItemAuto(ClientAddRequest);

	//if (EntryIds.IsEmpty()) //Ŭ�󿡼� �������н� ������û ����
	//{

	//	return;
	//}

	ServerAddItemAuto(ClientAddRequest);
}

void UATGInventoryComponent::ServerAddItemAuto_Implementation(FClientAddRequest ClientAddRequest)
{
	FInventoryChangeResult InventoryChangeResult;
	TArray<int32> EntryIds = AddItemAuto(ClientAddRequest);
	if (!EntryIds.IsEmpty())
	{
		InventoryChangeResult.Status = EInventoryChangeStatus::Success;
	}
	else
	{
		InventoryChangeResult.Status = EInventoryChangeStatus::Rejected;
		InventoryChangeResult.Reason = EInventoryRejectReason::Unknown;
	}

	InventoryChangeResult.PredictionKey = ClientAddRequest.PredictionKey; //���� Ŭ���̾�Ʈ ��Ī Ű

	InventoryChangeResult.NewEntryIds = EntryIds;
	
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
	Result.NewEntryIds.Add(EntryId);
	ClientMoveResult(Result);
}

void UATGInventoryComponent::ClientMoveResult_Implementation(const FInventoryChangeResult& Result)
{
	if (Result.Status == EInventoryChangeStatus::Rejected)
	{
		Inventory.PreviewRemoveById(Result.NewEntryIds[0]);
		OnItemPreRemoved.Broadcast(Result.NewEntryIds[0]);
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