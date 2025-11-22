// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGContainerComponent.h"
#include "ATGItemData.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include <Kismet/GameplayStatics.h>
#include <ATGItem.h>
#include "ATGPickupComponent.h"

// Sets default values for this component's properties
UATGContainerComponent::UATGContainerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// ...
}


// Called when the game starts
void UATGContainerComponent::BeginPlay()
{
	Super::BeginPlay();
	//ContainerInventory.Owner = TScriptInterface<IATGInventoryOwnerInterface>(this);
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

	if (IsLocallyOwned())
	{
		ServerMoveOrSwap(EntryId, NewX, NewY, bIsRotate);
	}
}

void UATGContainerComponent::TrySplitStack(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate, int32 SplitNum)
{
	ServerSplitStack(EntryId, NewX, NewY, bIsRotate, SplitNum);
}

void UATGContainerComponent::TryDropItem(int32 EntryId, int32 SplitNum)
{
	ServerDropItem(EntryId, SplitNum);
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
		UE_LOG(LogTemp, Warning, TEXT("UATGContainerComponent::ServerSpawnItem FInventoryEntry* Entry is Invaild"));
		return;
	}
	if (Entry->Item.Get()) // load
	{
		Entry->Item.LoadSynchronous();
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
	if (RemoveQty > 0)
	{
		ContainerInventory.DecreaseQtyAndRemoveById(EntryId, RemoveQty);
	}
}

void UATGContainerComponent::ServerSplitStack_Implementation(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate, int32 SplitNum)
{
	int32 Qty = SplitNum;
	FInventoryEntry* E = ContainerInventory.GetById(EntryId);

	//해당 셀에 새 아이템 추가 시도
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

