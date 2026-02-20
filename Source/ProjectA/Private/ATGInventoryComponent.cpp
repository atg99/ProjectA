// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "InventoryTypes.h"
#include "Data/ATGItemData.h"
#include "Data/ATGConsumableItemData.h"
#include "ATGItem.h"
#include "GameFramework/PlayerState.h"
#include "ATGPickupComponent.h"
#include <Kismet/GameplayStatics.h>
#include "ATGContainerComponent.h"
#include "Utils/NetworkUtil.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#include "Utils/ATGSerializationLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Widget/ATGItemContextMenuWidget.h"
#include "ATGInventoryItemWidget.h"
#include "ATGEnum.h"
#include "ATGItemObject.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Item_Use, "Event.Item.Use", "event when use consumableitem");

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
	// OnItemChanged.AddDynamic(this, &UATGInventoryComponent::HandleReplicatedChange);
	// ...

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
					ContextMenuWidget->AddToViewport(100); // Z-Order를 높게 설정하여 최상단 노출
					ContextMenuWidget->SetVisibility(ESlateVisibility::Collapsed); // 일단 숨김
				}
			}
		}
	}
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
	RepParams.Condition = ELifetimeCondition::COND_OwnerOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(UATGInventoryComponent, GridSize, RepParams);

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

void UATGInventoryComponent::OnRep_GridSize()
{
	Inventory.GridWidth = GridSize.X;
	Inventory.GridHeight = GridSize.Y;

	OnRebuildAll.Broadcast(-1);
}

void UATGInventoryComponent::ItemRemoved(int32 EntryId)
{
	NET_LOG(TEXT(""));
	OnItemRemoved.Broadcast(EntryId);
}

TArray<int32> UATGInventoryComponent::AddItemAuto(FClientAddRequest& ClientAddRequest, AActor* InteractedActor)
{
	NET_LOG(TEXT(""));
	TArray<int32> EntryIds;
	EntryIds.Empty();
	if (!ClientAddRequest.ItemDef.Get()) // �ε�
	{
		NET_LOG(TEXT("!ItemDef.Get()"));
		if (!ClientAddRequest.ItemDef.LoadSynchronous())
		{
			NET_LOG(FString::Printf(TEXT("!ClientAddRequest.ItemDef.LoadSynchronous() %s"), *ClientAddRequest.ItemDef.GetAssetName()));
			return EntryIds;
		}
	}

	if (!ClientAddRequest.ItemDef || ClientAddRequest.Quantity <= 0)
	{
		if (ClientAddRequest.Quantity <= 0)
		{
			NET_LOG(TEXT("ClientAddRequest.Quantity <= 0"));
		}
		else
		{
			NET_LOG(TEXT("!ClientAddRequest.ItemDef "));
		}
		return EntryIds;
	}

	int32 W = ClientAddRequest.ItemDef->Width;
	int32 H = ClientAddRequest.ItemDef->Height;
	int32 OutX = -1, OutY = -1;
	int32 OriginQty = ClientAddRequest.Quantity;
	int32 Qty = ClientAddRequest.Quantity;

	if (!Inventory.FindFirstFit(ClientAddRequest.ItemDef, W, H, OutX, OutY, Qty)) //���⼭ �����ϴ� ���ÿ� ���� ���� �� Qty ������ ��ȯ
	{
		if (IsHasAuthority()) // Decrease WorldItem Qty
		{
			if (auto Comp = GetPickupComp(InteractedActor))
			{
				NET_LOG(TEXT("DecreaseQty"));
				Comp->DecreaseQty(OriginQty - Qty);
			}
			ClientAddRequest.Quantity = Qty;
		}
		NET_LOG(TEXT("!FindFirstFit"));
		return EntryIds; // ���ο� �ڸ� ���� 
	}

	if (Qty <= 0) //������ 0�� �Ȱ�� 
	{
		if (IsHasAuthority()) // Decrease WorldItem Qty
		{
			if (auto Comp = GetPickupComp(InteractedActor))
			{
				Comp->DecreaseQty(OriginQty - Qty);
			}
			ClientAddRequest.Quantity = Qty;
		}
		NET_LOG(TEXT("Qty <= 0"));
		return EntryIds;
	}

	int32 Id = Inventory.AddItemAt(ClientAddRequest.ItemDef, Qty, OutX, OutY, W, H, false, ClientAddRequest.PredictionKey);
	EntryIds.Add(Id);
	//Qty ���� ��ȯ
	while (Qty >= 1) //������ 0�� �ɶ� ���� �ݺ�
	{
		OutX = -1;
		OutY = -1;
		if (!Inventory.FindFirstFit(W, H, OutX, OutY)) //�ٽ� �ڸ� �˻�, �����ϴ� ���� ���� X 
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
	NET_LOG(TEXT(""));
	//������ ���� ������ ���� �� �������� ����
	int32 OriginQty = ClientAddRequest.Quantity;
	int32 Qty = ClientAddRequest.Quantity;

	if (ClientAddRequest.ItemDef.IsNull())
	{
		NET_LOG(TEXT("ItemDef.IsNull()"));
		return;
	}

	if (!ClientAddRequest.ItemDef.IsValid())
	{
		NET_LOG(TEXT("!ClientAddRequest.ItemDef.IsValid()"));
		if (!ClientAddRequest.ItemDef.LoadSynchronous())
		{
			NET_LOG(TEXT("!ClientAddRequest.ItemDef.LoadSynchronous()"));
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
	//���⼭ ���� interface�� ������ ��������
	if (Inven)
	{
		Inven->TryHandleTransItemResult(OtherGridId, DecreasedQty);
	}
}

void UATGInventoryComponent::TryAddItemAt(TScriptInterface<IATGInventoryOwnerInterface> Inven, int32 OtherGridId, TSoftObjectPtr<UATGItemData> ItemDef, int32 InQty, int32 X, int32 Y, bool bRotate)
{
	NET_LOG(TEXT(""));
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
	NET_LOG(TEXT(""));
	if (!IsLocallyOwned())
	{
		NET_LOG(TEXT(" !IsLocallyOwned() Return"));
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

	//ȣ��Ʈ���� �ߺ����� ����
	if (!IsHasAuthority())
	{
		ServerAddItemAuto(ClientAddRequest, InteractActor);
	}
}

void UATGInventoryComponent::ServerAddItemAuto_Implementation(FClientAddRequest ClientAddRequest, AActor* InteractedActor)
{
	NET_LOG(TEXT(""));
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
	//	return; // ���� �� ������ ���� ȣ�� ����
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
	if (!E)
	{
		NET_LOG(TEXT(" E is Invaild"));
		return;
	}

	//�ش� ���� �� ������ �߰� �õ�
	if (Inventory.AddItemAt(E->Item, Qty, NewX, NewY, E->Width, E->Height, bIsRotate, -1))
	{
		//������ ������ ������ŭ ���� ���� ����
		Inventory.DecreaseQtyAndRemoveById(EntryId, SplitNum-Qty);
	}
	else
	{
		//���н� �ش� ���� �����۰� ���� �õ�
		//MergeStackAt���� ��������ó�� ���Ե�
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
	// ItemBPClass�� ���������� �ʴٸ� Lobby Drop X
	if (!ItemBPClass)
	{
		return;
	}

	ServerSpawnItem(EntryId, SplitNum);
	if (SplitNum > 0)
	{
		//������ ��������
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

void UATGInventoryComponent::ServerDecreaseItem_Implementation(int32 EntryId, int32 Qty)
{
	Inventory.DecreaseQtyAndRemoveById(EntryId, Qty);
		//OnItemRemoved.Broadcast(EntryId);
}

void UATGInventoryComponent::ServerSpawnItem_Implementation(int32 EntryId, int32 SplitNum)
{
	// ItemBPClass�� ���������� �ʴٸ� Lobby Spawn X
	if (!ItemBPClass)
	{
		return;
	}

	FInventoryEntry* Entry = Inventory.GetById(EntryId);
	if (!Entry)
	{
		UE_LOG(LogTemp, Warning, TEXT("UATGInventoryComponent::ServerSpawnItem FInventoryEntry* Entry is Invaild"));
		return;
	}

	if (Entry->Item.IsNull())
	{
		NET_LOG(TEXT("Entry->Item.IsNull()"));
		return;
	}

	if (!Entry->Item.IsValid())
	{
		if (!Entry->Item.LoadSynchronous())
		{
			NET_LOG(TEXT("!Entry->Item.LoadSynchronous()"));
			return;
		}
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
		//Inventory.PreviewRemoveById(E->PredictionKey); // PredKey�� ���� ����
		//OnItemPreRemoved.Broadcast(E->PredictionKey);  // �������Ե� �˷� ����
	}
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("!!! InventComp HandleReplicatedAddp"));
}

void UATGInventoryComponent::UseItem(const FATGItemInfo& ItemInfo)
{
	AActor* OwnerActor = GetOwner();
	APawn* AvatarPawn = nullptr;

	if (APlayerState* PS = Cast<APlayerState>(OwnerActor))
	{
		AvatarPawn = PS->GetPawn();
	}
	else
	{
		return;
	}

	if (!AvatarPawn)
	{
		return;
	}

	UATGItemData* ItemData = ItemInfo.ItemDef.Get();
	if (!ItemData)
	{
		ItemData = ItemInfo.ItemDef.LoadSynchronous();
		ensure(ItemData);
	}
	UATGConsumableItemData* ConsumableItemData = Cast<UATGConsumableItemData>(ItemData);

	FGameplayEventData Payload;
	Payload.EventMagnitude = ItemInfo.EntryId;
	Payload.Instigator = AvatarPawn; // Instigator를 Pawn으로 설정
	Payload.Target = AvatarPawn;     // Target도 Pawn
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

	// 데이터 세팅

	ContextMenuWidget->InitMenu(ItemInfo);

	// 위치 설정 (마우스 위치로 이동)
	ContextMenuWidget->SetPositionInViewport(ScreenPosition, true);

	// 보이게 설정
	ContextMenuWidget->SetVisibility(ESlateVisibility::Visible);

}

//void UATGInventoryComponent::HandleReplicatedChange(int32 EntryId)
//{
//	Inventory.GetById(EntryId);
//}

bool UATGInventoryComponent::IsHasAuthority()
{
	return (GetOwner() && GetOwner()->HasAuthority());
}

bool UATGInventoryComponent::IsLocallyOwned()
{
	GetOwner();
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

// �������� 
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
		if (UActorComponent* Comp = InteractedActor->GetComponentByClass(UATGPickupComponent::StaticClass()))
		{
			return Cast<UATGPickupComponent>(Comp);
		}
	}

	return nullptr;
}

