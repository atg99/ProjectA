// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGContainerComponent.h"
#include "Data/ATGItemData.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include <Kismet/GameplayStatics.h>
#include <ATGItem.h>
#include "ATGPickupComponent.h"
#include "Utils/NetworkUtil.h"

// Sets default values for this component's properties
UATGContainerComponent::UATGContainerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	ContainerInventory.GridWidth = GridWidth;
	//ContainerInventory.GridHeight = GridHeight;
}

// Called when the game starts
void UATGContainerComponent::BeginPlay()
{
	Super::BeginPlay();

	ContainerInventory.GridWidth = GridWidth;
	ContainerInventory.GridHeight = GridHeight;
	ContainerInventory.Owner = TScriptInterface<IATGInventoryOwnerInterface>(this);

	for (auto& Item : ContainerItems)
	{
		if (!Item.ItemDef.Get())
		{
			Item.ItemDef.LoadSynchronous();
		}
	}

	//!!! if don't initialize check owner's replication property
	if (GetOwner()->HasAuthority())
	{
		InitContainerItem();
		//ContainerInventory.SortEntryByItemId();
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
	//수정예정
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
	//get component's owner (actor) -> get actor's owner (character) -> get character's owner (playercontroller) -> is localcontroller?
	if (APlayerController* PC = GetOwner()->GetOwner() ? Cast<APlayerController>(GetOwner()->GetOwner()->GetOwner()) : nullptr)
	{
		//UE_LOG(LogTemp, Display, TEXT("IsLocallyOwned : Get PC"));
		return  PC->IsLocalController();
	}
	//UE_LOG(LogTemp, Display, TEXT("IsLocallyOwned : false"));
	return false;
}

bool UATGContainerComponent::CheckCanMove(int32 StartX, int32 StartY, int32 W, int32 H, int32 IgnoreId)
{
	return ContainerInventory.CheckMoveOrSwap(StartX, StartY, W, H, IgnoreId);
}

void UATGContainerComponent::TryMoveOrSwapClient(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
	UE_LOG(LogTemp, Display, TEXT("UATGContainerComponent::TryMoveOrSwapClient"));

	//if (IsLocallyOwned())

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
	//여기서 받은 interface로 아이템 수량감소
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

	if (!ClientAddRequest.ItemDef.Get()) // 로드
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
	int32 OriginQty = ClientAddRequest.Quantity;

	int32 RemainingQty = ClientAddRequest.Quantity;

	if (!ContainerInventory.FindFirstFit(ClientAddRequest.ItemDef, W, H, OutX, OutY, RemainingQty)) //여기서 존재하는 스택에 저장 남은 값 Qty 참조로 반환
	{
		if (GetOwner()->HasAuthority()) // Decrease WorldItem Qty
		{
			ClientAddRequest.Quantity = RemainingQty;
		}
		return EntryIds; // 새로운 자리 없음 
	}

	if (RemainingQty <= 0) //수량이 0이 된경우
	{
		if (GetOwner()->HasAuthority()) // Decrease WorldItem Qty
		{
			ClientAddRequest.Quantity = RemainingQty;
		}
		return EntryIds;
	}

	int32 Id = ContainerInventory.AddItemAt(ClientAddRequest.ItemDef, RemainingQty, OutX, OutY, W, H, false, ClientAddRequest.PredictionKey);
	EntryIds.Add(Id);
	//RemainingQty 참조 반환
	while (RemainingQty >= 1) //수량이 0이 될때 까지 반복
	{
		OutX = -1;
		OutY = -1;
		if (!ContainerInventory.FindFirstFit(W, H, OutX, OutY)) //다시 자리 검색, 존재하는 스택 저장 X 
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

	if (GetOwner()->HasAuthority()) // Decrease WorldItem Qty 
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
		//아이템 수량감소 및 삭제
		ContainerInventory.DecreaseQtyAndRemoveById(EntryId, SplitNum);
		return;
	}
	ServerRemoveItem(EntryId);
	return;
}

void UATGContainerComponent::ServerSpawnItem_Implementation(int32 EntryId, int32 SplitNum)
{
	FInventoryEntry* Entry = ContainerInventory.GetById(EntryId);
	if (!Entry)
	{
		NET_LOG(TEXT("Entry is Invaild"));
		return;
	}

	if (Entry->Item.IsNull())
	{
		NET_LOG(TEXT("Entry->Item.IsNull()"));
		return;
	}
	if (!Entry->Item.IsValid())
	{
		Entry->Item.LoadSynchronous();
	}

	// 로드 실패 시 스폰 불가
	if (!Entry->Item.IsValid())
	{
		NET_LOG(TEXT("Entry Item Load Failed"));
		return;
	}

	if (ACharacter* PlayerCharacter = Cast<ACharacter>(GetOwner()->GetOwner()))
	{
		FVector SpawnLoc = PlayerCharacter->GetActorLocation() + FVector(100.f, 0, -50.f);

		FTransform SpawnTransform = { FRotator::ZeroRotator, SpawnLoc, FVector(1.f) };

		FActorSpawnParameters SpawnParam;
		SpawnParam.Owner = PlayerCharacter;

		AATGItem* ItemActor = GetWorld()->SpawnActorDeferred<AATGItem>(ItemBPClass, SpawnTransform, PlayerCharacter, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, ESpawnActorScaleMethod::OverrideRootScale);
		if (ItemActor)
		{
			if (ItemActor->GetPickupComp())
			{
				ItemActor->GetPickupComp()->ItemDef = Entry->Item;
				ItemActor->GetPickupComp()->ItemQty = SplitNum > 0 ? SplitNum : Entry->Quantity;
				UGameplayStatics::FinishSpawningActor(ItemActor, SpawnTransform);
				UE_LOG(LogTemp, Warning, TEXT("Spawn Item ItemActor->GetPickupComp() Is Valid"));
			}
			else
			{
				ItemActor->Destroy();
				UE_LOG(LogTemp, Warning, TEXT("Spawn Item ItemActor->GetPickupComp() == nullptr"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UATGContainerComponent :: Spawn Item ItemActor == nullptr"));
		}
	}
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
		NET_LOG(TEXT("Entry is Invaild"));
		return;
	}

	if (!E->Item.IsValid())
	{
		if (E->Item.LoadSynchronous())
		{
			NET_LOG(TEXT("E->Item.LoadSynchronous() Invaild"));
			return;
		}
	}

	if (ContainerInventory.AddItemAt(E->Item, Qty, NewX, NewY, E->Width, E->Height, bIsRotate, -1))
	{
		//성공시 성공한 수량만큼 원본 스텍 감소
		ContainerInventory.DecreaseQtyAndRemoveById(EntryId, SplitNum - Qty);
	}
	else
	{
		//실패시 해당 셀의 아이템과 병합 시도
		//MergeStackAt에서 수량감소처리 포함됨
		ContainerInventory.MergeStackAtAndDecrease(*E, SplitNum, NewX, NewY, bIsRotate);
	}
}

void UATGContainerComponent::ServerMoveOrSwap_Implementation(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
	bool S = ContainerInventory.MoveOrSwap(EntryId, NewX, NewY, bIsRotate);
	UE_LOG(LogTemp, Warning, TEXT("UATGContainerComponent::ServerMoveOrSwap %d"), S);
}

