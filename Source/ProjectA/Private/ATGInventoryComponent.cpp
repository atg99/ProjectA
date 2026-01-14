// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "InventoryTypes.h"
#include "Data/ATGItemData.h"
#include "ATGItem.h"
#include "GameFramework/PlayerState.h"
#include "ATGPickupComponent.h"
#include <Kismet/GameplayStatics.h>
#include "ATGContainerComponent.h"
#include "Utils/NetworkUtil.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"


// Sets default values for this component's properties
UATGInventoryComponent::UATGInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	//Inventory.OwnerComp = this;
	Inventory.Owner = TScriptInterface<IATGInventoryOwnerInterface>(this);

	SetIsReplicatedByDefault(true);
	
}


// Called when the game starts
void UATGInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	//Inventory.OwnerComp = this;
	Inventory.Owner = TScriptInterface<IATGInventoryOwnerInterface>(this);

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
	FDoRepLifetimeParams RepParams;
	RepParams.bIsPushBased = true;
	//DOREPLIFETIME_WITH_PARAMS_FAST(UATGInventoryComponent, Inventory, RepParams);
	DOREPLIFETIME_CONDITION(UATGInventoryComponent, Inventory, COND_OwnerOnly);
}

//void UATGInventoryComponent::RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags)
//{
//	UE::Net::FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(this, Context, RegistrationFlags);
//}

void UATGInventoryComponent::ItemAdded(int32 EntryId)
{
	NET_LOG(TEXT(""));
	OnItemAdded.Broadcast(EntryId);
}

void UATGInventoryComponent::ItemChanged(int32 EntryId)
{
	NET_LOG(TEXT(""));
	OnItemChanged.Broadcast(EntryId);
}

void UATGInventoryComponent::InventoryForceNetUpdate()
{
	GetOwner()->ForceNetUpdate();
}

void UATGInventoryComponent::ItemRemoved(int32 EntryId)
{
	NET_LOG(TEXT(""));
	OnItemRemoved.Broadcast(EntryId);
}

TArray<int32> UATGInventoryComponent::AddItemAuto(FClientAddRequest& ClientAddRequest, AActor* InteractedActor)
{

	TArray<int32> EntryIds;
	EntryIds.Empty();
	if (!ClientAddRequest.ItemDef || ClientAddRequest.Quantity <= 0) return EntryIds;

	if (!ClientAddRequest.ItemDef.Get()) // 로드
	{
		ClientAddRequest.ItemDef.LoadSynchronous();
	}

	int32 W = ClientAddRequest.ItemDef->Width;
	int32 H = ClientAddRequest.ItemDef->Height;
	int32 OutX = -1, OutY = -1;
	int32 OriginQty = ClientAddRequest.Quantity;
	int32 Qty = ClientAddRequest.Quantity;

	if (!Inventory.FindFirstFit(ClientAddRequest.ItemDef, W, H, OutX, OutY, Qty)) //여기서 존재하는 스택에 저장 남은 값 Qty 참조로 반환
	{
		if (IsHasAuthority()) // Decrease WorldItem Qty
		{
			if (auto Comp = GetPickupComp(InteractedActor))
			{
				Comp->DecreaseQty(OriginQty - Qty);
			}
			ClientAddRequest.Quantity = Qty;
		}
		return EntryIds; // 새로운 자리 없음 
	}

	if (Qty <= 0) //수량이 0이 된경우 
	{
		if (IsHasAuthority()) // Decrease WorldItem Qty
		{
			if (auto Comp = GetPickupComp(InteractedActor))
			{
				Comp->DecreaseQty(OriginQty - Qty);
			}
			ClientAddRequest.Quantity = Qty;
		}
		return EntryIds;
	}

	int32 Id = Inventory.AddItemAt(ClientAddRequest.ItemDef, Qty, OutX, OutY, W, H, false, ClientAddRequest.PredictionKey);
	EntryIds.Add(Id);
	//Qty 참조 반환
	while (Qty >= 1) //수량이 0이 될때 까지 반복
	{
		OutX = -1;
		OutY = -1;
		if (!Inventory.FindFirstFit(W, H, OutX, OutY)) //다시 자리 검색, 존재하는 스택 저장 X 
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

	if (IsHasAuthority()) // Decrease WorldItem Qty 
	{
		if (auto Comp = GetPickupComp(InteractedActor))
		{
			Comp->DecreaseQty(OriginQty - Qty);
		}
		ClientAddRequest.Quantity = Qty;
	}

	return EntryIds;
}

void UATGInventoryComponent::ServerAddItemAt_Implementation(FClientAddRequest ClientAddRequest, int32 OtherGridId, const TScriptInterface<IATGInventoryOwnerInterface>& Inven)
{
	//아이템 이전 데이터 복제 후 원본에서 삭제
	int32 OriginQty = ClientAddRequest.Quantity;
	int32 Qty = ClientAddRequest.Quantity;

	Inventory.AddItemAt(ClientAddRequest.ItemDef, Qty, ClientAddRequest.X, ClientAddRequest.Y, ClientAddRequest.ItemDef->Width, ClientAddRequest.ItemDef->Height, ClientAddRequest.bRotated);

	if (Qty > 0)
	{
		ClientAddRequest.Quantity = Qty;
		AddItemAuto(ClientAddRequest, nullptr);
		Qty = ClientAddRequest.Quantity;
	}

	int32 DecreasedQty = OriginQty - Qty;
	//여기서 받은 interface로 아이템 수량감소
	if (Inven)
	{
		Inven->TryHandleTransItemResult(OtherGridId, DecreasedQty);
	}
}

void UATGInventoryComponent::TryAddItemAt(TScriptInterface<IATGInventoryOwnerInterface> Inven, int32 OtherGridId, TSoftObjectPtr<UATGItemData> ItemDef, int32 InQty, int32 X, int32 Y, bool bRotate)
{
	//ServerAddItemAt(ItemDef, InQty, X, Y, bRotate);
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

	TArray<int32> EntryIds = AddItemAuto(ClientAddRequest);

	ServerAddItemAuto(ClientAddRequest, InteractActor);
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

	InventoryChangeResult.PredictionKey = ClientAddRequest.PredictionKey; //서버 클라이언트 매칭 키

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
		// 성공이면 복제 도착 시점(HandleReplicatedAdd)에서 지우므로 생략
	}
	else
	{
		// 실패 → 프리뷰 즉시 제거
		if (Result.PredictionKey != 0)
		{
			//Inventory.PreviewRemoveById(Result.PredictionKey);
			//OnItemPreRemoved.Broadcast(Result.PredictionKey);
		}
	}
}

void UATGInventoryComponent::TryMoveOrSwapClient(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
	if (!IsLocallyOwned())
	{
		return;
	}

	//if (!Inventory.PreviewMoveOrSwap(EntryId, NewX, NewY, bIsRotate))
	//{
	//	return; // 놓을 수 없으면 서버 호출 안함
	//}
	
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

	//해당 셀에 새 아이템 추가 시도
	if (Inventory.AddItemAt(E->Item, Qty, NewX, NewY, E->Width, E->Height, bIsRotate, -1))
	{
		//성공시 성공한 수량만큼 원본 스텍 감소
		Inventory.DecreaseQtyAndRemoveById(EntryId, SplitNum-Qty);
	}
	else
	{
		//실패시 해당 셀의 아이템과 병합 시도
		//MergeStackAt에서 수량감소처리 포함됨
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
	ServerSpawnItem(EntryId, SplitNum);
	if (SplitNum > 0)
	{
		//아이템 수량감소
		Inventory.DecreaseQtyAndRemoveById(EntryId, SplitNum);
		return;
	}
	ServerRemoveItem(EntryId);
	return;
}

void UATGInventoryComponent::ServerMoveOrSwap_Implementation(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
	UE_LOG(LogTemp, Warning, TEXT("UATGInventoryComponent::ServerMoveOrSwap %d :"), bIsRotate)
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
		//Inventory.PreviewRemoveById(Result.NewEntryIds[0]);
		//OnItemPreRemoved.Broadcast(Result.NewEntryIds[0]);
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

void UATGInventoryComponent::ServerSpawnItem_Implementation(int32 EntryId, int32 SplitNum)
{
	FInventoryEntry* Entry = Inventory.GetById(EntryId);
	if (!Entry)
	{
		UE_LOG(LogTemp, Warning, TEXT("UATGInventoryComponent::ServerSpawnItem FInventoryEntry* Entry is Invaild"));
		return;
	}
	if (Entry->Item.Get()) // load
	{
		Entry->Item.LoadSynchronous();
	}

	if (APlayerState* PS = Cast<APlayerState>(GetOwner()))
	{
		FVector SpawnLoc =  PS->GetPawn()->GetActorLocation() + FVector(100.f, 0, -50.f);

		FTransform SpawnTransform = { FRotator::ZeroRotator, SpawnLoc, FVector(1.f) };

		FActorSpawnParameters SpawnParam;
		SpawnParam.Owner = PS->GetPawn();

		AATGItem* ItemActor = GetWorld()->SpawnActorDeferred<AATGItem>(ItemBPClass, SpawnTransform, PS->GetPawn(), nullptr,
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
	}
}

void UATGInventoryComponent::HandleReplicatedAdd(int32 EntryId)
{
	const FInventoryEntry* E = Inventory.GetById(EntryId);
	if (E && E->PredictionKey != 0)
	{
		//Inventory.PreviewRemoveById(E->PredictionKey); // PredKey로 직접 제거
		//OnItemPreRemoved.Broadcast(E->PredictionKey);  // 위젯에게도 알려 제거
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

UATGPickupComponent* UATGInventoryComponent::GetPickupComp(AActor* InteractedActor)
{
	if (InteractedActor)
	{
		if (UActorComponent* Comp = InteractedActor->GetComponentByClass(UATGPickupComponent::StaticClass()))
		{
			return Cast<UATGPickupComponent>(Comp);
		}
	}

	return nullptr;
}
