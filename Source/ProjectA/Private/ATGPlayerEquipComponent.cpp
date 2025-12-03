// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGPlayerEquipComponent.h"
#include "ATGEquipmentComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "Data/ATGItemData.h"
#include "Data/ATGWeaponData.h"
#include "Kismet/GameplayStatics.h" 
#include "Engine/World.h"
#include "Weapon/ATGWeaponBase.h"
#include "Net/UnrealNetwork.h"
#include "Components/SceneComponent.h"


// Sets default values for this component's properties
UATGPlayerEquipComponent::UATGPlayerEquipComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);

    //MainWeapon1 슬롯 추가
    FEquipmentSlot Slot1;
    Slot1.SlotType = EEquipmentSlotType::MainWeapon1;
    Slot1.EquippedActor = nullptr; // 초기엔 장비 없음
    EquipmentSlots.Add(Slot1);

    FEquipmentSlot Slot2;
    Slot2.SlotType = EEquipmentSlotType::MainWeapon2;
    Slot2.EquippedActor = nullptr;
    EquipmentSlots.Add(Slot2);

}


// Called when the game starts
void UATGPlayerEquipComponent::BeginPlay()
{
	Super::BeginPlay();

    // PlayerState가 이미 있다면 바로 초기화
    if (CheckPlayerStateCompReady())
    {
        
    }
    else
    {
        GetWorld()->GetTimerManager().SetTimer(
            TimerHandle_InitCheck,
            FTimerDelegate::CreateWeakLambda(this, [this]() //this 유효성 검사
                {
                    // 람다 내부에서 함수 호출
                    CheckPlayerStateCompReady();
                }),
            0.1f,
            true
        );
    }

}

void UATGPlayerEquipComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(UATGPlayerEquipComponent, CurrentUsingSlot, COND_None);
    DOREPLIFETIME_CONDITION(UATGPlayerEquipComponent, EquipmentSlots, COND_None);
}

bool UATGPlayerEquipComponent::CheckPlayerStateCompReady()
{
    APlayerState* PS = GetOwningPlayerCharacter()->GetPlayerState();
    if (PS)
    {
        UATGEquipmentComponent* EqComp = Cast<UATGEquipmentComponent>(PS->GetComponentByClass(UATGEquipmentComponent::StaticClass()));
        if (EqComp)
        {
            InitEquipComponent(EqComp);
            GetWorld()->GetTimerManager().ClearTimer(TimerHandle_InitCheck);
            return true;
        }
    }

    return false;
}

void UATGPlayerEquipComponent::InitEquipComponent(UATGEquipmentComponent* EquipmentComponent)
{
    if (EquipmentComponent)
    {
        UE_LOG(LogTemp, Log, TEXT("PlayerState Ready! Component Initialized."));
        EquipmentComponent->OnFirstMainWeaponChanged.AddDynamic(this, &UATGPlayerEquipComponent::HandleFirstMainWeaponChanged);

        EquipmentComponent->OnSecondMainWeaponChanged.AddDynamic(this, &UATGPlayerEquipComponent::HandleSecondMainWeaponChanged);
    }
}

void UATGPlayerEquipComponent::HandleFirstMainWeaponChanged(FInventoryEntry InFirstMainWeapon)
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    if (EquipmentSlots[0].EquippedActor)
    {
        EquipmentSlots[0].EquippedActor->Destroy();
        EquipmentSlots[0].EquippedActor = nullptr;
    }

    UATGItemData* ItemData = InFirstMainWeapon.Item.Get();
    if (!ItemData) return;

    UATGWeaponData* WeaponData = Cast<UATGWeaponData>(ItemData);
    if (!WeaponData || !WeaponData->WeaponClass) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    UWorld* World = GetWorld();
    if (!World) return;

 
    FTransform SpawnTransform = OwnerCharacter->GetActorTransform();

    AATGWeaponBase* SpawnedActor = World->SpawnActorDeferred<AATGWeaponBase>(
        WeaponData->WeaponClass,
        SpawnTransform,
        OwnerCharacter, // Owner
        OwnerCharacter, // Instigator
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn
    );

    if (SpawnedActor)
    {
        SpawnedActor->SetReplicates(true);
        EquipmentSlots[0].EquippedActor = SpawnedActor;

        //BeginPlay 및 초기화 실행 
        UGameplayStatics::FinishSpawningActor(SpawnedActor, SpawnTransform);

        FName AttachSocketName = CurrentUsingSlot == EEquipmentSlotType::MainWeapon1 ? SniperSocketName : Main1BackSocketName;

        if (USceneComponent* AttachComp = GetSlaveMesh())
        {
            SpawnedActor->AttachToComponent(AttachComp, FAttachmentTransformRules::SnapToTargetIncludingScale, AttachSocketName);
        }
    }
}

void UATGPlayerEquipComponent::HandleSecondMainWeaponChanged(FInventoryEntry InSecondMainWeapon)
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    if (EquipmentSlots[1].EquippedActor)
    {
        EquipmentSlots[1].EquippedActor->Destroy();
        EquipmentSlots[1].EquippedActor = nullptr;
    }

    UATGItemData* ItemData = InSecondMainWeapon.Item.Get();
    if (!ItemData) return;

    UATGWeaponData* WeaponData = Cast<UATGWeaponData>(ItemData);
    if (!WeaponData || !WeaponData->WeaponClass) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    UWorld* World = GetWorld();
    if (!World) return;

    FTransform SpawnTransform = OwnerCharacter->GetActorTransform();

    AATGWeaponBase* SpawnedActor = World->SpawnActorDeferred<AATGWeaponBase>(
        WeaponData->WeaponClass,
        SpawnTransform,
        OwnerCharacter, // Owner
        OwnerCharacter, // Instigator
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn
    );

    if (SpawnedActor)
    {
        SpawnedActor->SetReplicates(true);
        EquipmentSlots[1].EquippedActor = SpawnedActor;
       
        // BeginPlay 및 초기화 실행 
        UGameplayStatics::FinishSpawningActor(SpawnedActor, SpawnTransform);

        FName AttachSocketName = CurrentUsingSlot == EEquipmentSlotType::MainWeapon2 ? SniperSocketName : Main2BackSocketName;

        if (USceneComponent* AttachComp = GetSlaveMesh())
        {
            SpawnedActor->AttachToComponent(AttachComp, FAttachmentTransformRules::SnapToTargetIncludingScale, AttachSocketName);
        }
    }
}

// Called every frame
void UATGPlayerEquipComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UATGPlayerEquipComponent::ServerChangePlayerUsingSlot_Implementation(EEquipmentSlotType TryUsingSlot)
{
    UE_LOG(LogTemp, Log, TEXT("ServerChangePlayerUsingSlot_Implementation"));
    CurrentUsingSlot = TryUsingSlot;
    //서버에서 attach하면 동기화됨 클라에서 불필요
    ChangeWeaponEquip();
}

void UATGPlayerEquipComponent::OnRep_CurrentUsingSlot()
{
    UE_LOG(LogTemp, Log, TEXT("UATGPlayerEquipComponent::OnRep_CurrentUsingSlot"));
    //서버에서 attach하면 동기화됨 클라에서 불필요
    //ChangeWeaponEquip();
}

void UATGPlayerEquipComponent::ChangeWeaponEquip()
{
    if (!GetSlaveMesh())
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("UATGPlayerEquipComponent::ChangeWeaponEquip"));

    switch (CurrentUsingSlot)
    {
    case EEquipmentSlotType::MainWeapon1:
    {
        if (EquipmentSlots[0].EquippedActor)
        {
            EquipmentSlots[0].EquippedActor->AttachToComponent(
                GetSlaveMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                SniperSocketName
            );
        }
        if (EquipmentSlots[1].EquippedActor)
        {
            EquipmentSlots[1].EquippedActor->AttachToComponent(
                GetSlaveMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                Main2BackSocketName
            );
        }
        break;
    }
    case EEquipmentSlotType::MainWeapon2:
    {
        if (EquipmentSlots[0].EquippedActor)
        {
            EquipmentSlots[0].EquippedActor->AttachToComponent(
                GetSlaveMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                Main1BackSocketName
            );
        }
        if (EquipmentSlots[1].EquippedActor)
        {
            EquipmentSlots[1].EquippedActor->AttachToComponent(
                GetSlaveMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                SniperSocketName
            );
        }
        break;
    }
    }
}

ACharacter* UATGPlayerEquipComponent::GetOwningPlayerCharacter()
{
    if (!GetOwner()) return nullptr;
	return Cast<ACharacter>(GetOwner());
}

USceneComponent* UATGPlayerEquipComponent::GetSlaveMesh()
{
    if (!GetOwningPlayerCharacter())
    {
        return nullptr;
    }

    TArray<USceneComponent*> Children;
    USceneComponent* AttachComp = nullptr;
    GetOwningPlayerCharacter()->GetMesh()->GetChildrenComponents(false, Children);
    for (auto Comp : Children)
    {
        if (Comp->ComponentHasTag(FName("SlaveMesh")))
        {
            return Comp;
        }
    }

    return nullptr;
}
