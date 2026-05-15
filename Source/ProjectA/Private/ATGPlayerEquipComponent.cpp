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
#include "Data/ATGRangeWeaponData.h"
#include "Data/ATGMeleeWeaponData.h"
#include "Net/UnrealNetwork.h"
#include "Components/SceneComponent.h"
#include "Utils/NetworkUtil.h"
#include "ATGPlayerCharacter.h"
#include "ATGPlayerController.h"
#include "Weapon/ATGRangeWeapon.h"
#include "Weapon/ATGMeleeWeapon.h"

UATGPlayerEquipComponent::UATGPlayerEquipComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}


void UATGPlayerEquipComponent::BeginPlay()
{
    Super::BeginPlay();

    EquipmentSlots.Empty();
    for (EEquipmentSlotType EquipmentSlotType : TEnumRange<EEquipmentSlotType>())
    {
        InitSlot(EquipmentSlotType);
    }

    if (!CheckPlayerStateCompReady())
    {
        GetWorld()->GetTimerManager().SetTimer(
            TimerHandle_InitCheck,
            FTimerDelegate::CreateWeakLambda(this, [this]()
                {
                    CheckPlayerStateCompReady();
                }),
            0.1f,
            true
        );
    }

    OnRep_CurrentUsingSlot();
}

void UATGPlayerEquipComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UATGPlayerEquipComponent, CurrentUsingSlot, COND_None);

    DOREPLIFETIME_CONDITION(UATGPlayerEquipComponent, EquipmentSlots, COND_None);
    DOREPLIFETIME_CONDITION(UATGPlayerEquipComponent, ATGCharacterInputState, COND_OwnerOnly);
}

bool UATGPlayerEquipComponent::CheckPlayerStateCompReady()
{
    const ACharacter* Character = GetOwningPlayerCharacter();
    APlayerState* PS = Character ? Character->GetPlayerState() : nullptr;
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
    if (!EquipmentComponent)
    {
        return;
    }

    EquipmentComponent->OnFirstMainWeaponChanged.RemoveDynamic(this, &UATGPlayerEquipComponent::HandleFirstMainWeaponChanged);
    EquipmentComponent->OnSecondMainWeaponChanged.RemoveDynamic(this, &UATGPlayerEquipComponent::HandleSecondMainWeaponChanged);

    EquipmentComponent->OnFirstMainWeaponChanged.AddDynamic(this, &UATGPlayerEquipComponent::HandleFirstMainWeaponChanged);
    EquipmentComponent->OnSecondMainWeaponChanged.AddDynamic(this, &UATGPlayerEquipComponent::HandleSecondMainWeaponChanged);
}

void UATGPlayerEquipComponent::HandleFirstMainWeaponChanged(FInventoryEntry InFirstMainWeapon)
{


    if (!GetOwner()->HasAuthority())
    {
        return;
    }

	FEquipmentSlot* Main1Slot = GetSlotByType(EEquipmentSlotType::MainWeapon1Slot);
    if (!Main1Slot)
    {
		checkNoEntry();
        return;
    }
    if (Main1Slot->EquippedActor)
    {
        Main1Slot->EquippedActor->Destroy();
        Main1Slot->EquippedActor = nullptr;
    }
    
    if (InFirstMainWeapon.Item.IsNull())
    {
        ClearSlot(*Main1Slot);
        return;
    }

    if (!InFirstMainWeapon.Item.IsValid())
    {
        if (!InFirstMainWeapon.Item.LoadSynchronous())
        {
            NET_LOG(TEXT("Item.LoadSynchronous() Fail"));
            return;
        }
    }

    UATGItemData* ItemData = InFirstMainWeapon.Item.Get();

    UATGWeaponData* WeaponData = ItemData ? Cast<UATGWeaponData>(ItemData) : nullptr;
    if (!WeaponData || !WeaponData->GetWeaponClass())
    {
        ClearSlot(*Main1Slot);
        return;
    }

    if (UATGRangeWeaponData* RangeWeaponData = Cast<UATGRangeWeaponData>(WeaponData))
    {
		SpawnRangeWeaponInSlot(*Main1Slot, RangeWeaponData);
    }
    else if(UATGMeleeWeaponData* MeleeWeaponData = Cast<UATGMeleeWeaponData>(WeaponData))
    {
        SpawnMeleeWeaponInSlot(*Main1Slot, MeleeWeaponData);
	}

}

void UATGPlayerEquipComponent::HandleSecondMainWeaponChanged(FInventoryEntry InSecondMainWeapon)
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    FEquipmentSlot* Main2Slot = GetSlotByType(EEquipmentSlotType::MainWeapon2Slot);
    if (!Main2Slot)
    {
        checkNoEntry();
        return;
    }
    if (Main2Slot->EquippedActor)
    {
        Main2Slot->EquippedActor->Destroy();
        Main2Slot->EquippedActor = nullptr;
    }

    if (InSecondMainWeapon.Item.IsNull())
    {
        ClearSlot(*Main2Slot);
        return;
    }

    if (!InSecondMainWeapon.Item.IsValid())
    {
        if (!InSecondMainWeapon.Item.LoadSynchronous())
        {
            NET_LOG(TEXT("Item.LoadSynchronous() Fail"));
            return;
        }
    }

    UATGItemData* ItemData = InSecondMainWeapon.Item.Get();

    UATGWeaponData* WeaponData = ItemData ? Cast<UATGWeaponData>(ItemData) : nullptr;
    if (!WeaponData || !WeaponData->GetWeaponClass())
    {
        ClearSlot(*Main2Slot);
        return;
    }

    if( UATGRangeWeaponData* RangeWeaponData = Cast<UATGRangeWeaponData>(WeaponData))
    {
        SpawnRangeWeaponInSlot(*Main2Slot, RangeWeaponData);
	}
    else if (UATGMeleeWeaponData* MeleeWeaponData = Cast<UATGMeleeWeaponData>(WeaponData))
    {
        SpawnMeleeWeaponInSlot(*Main2Slot, MeleeWeaponData);
    }
}

void UATGPlayerEquipComponent::ClearSlot(FEquipmentSlot& Slot)
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    if (Slot.EquippedActor)
    {
        Slot.EquippedActor->Destroy();
        Slot.EquippedActor = nullptr;
    }


    
    if (CurrentUsingSlot == Slot.SlotType)
    {
        ServerChangePlayerUsingSlot(EEquipmentSlotType::None);
    }
}


void UATGPlayerEquipComponent::TryChangePlayerUsingSlot(EEquipmentSlotType DesiredSlot)
{
    ServerChangePlayerUsingSlot(DesiredSlot);
    ChangePlayerUsingSlot(DesiredSlot);
}

void UATGPlayerEquipComponent::ServerChangePlayerUsingSlot_Implementation(EEquipmentSlotType DesiredSlot)
{
    ChangePlayerUsingSlot(DesiredSlot);
}

void UATGPlayerEquipComponent::ChangePlayerUsingSlot(EEquipmentSlotType DesiredSlot)
{
    FEquipmentSlot* TargetSlot = GetSlotByType(DesiredSlot);
    if (!TargetSlot)
    {
        return;
    }

    const bool bCanUseSlot = TargetSlot->EquippedActor || TargetSlot->SlotType == EEquipmentSlotType::None;
    if (!bCanUseSlot)
    {
        return;
    }

    CurrentUsingSlot = DesiredSlot;
    ChangeWeaponEquip();

    ACharacter* Character = GetOwningPlayerCharacter();
    if (!Character)
    {
        return;
    }

    if (Character->GetController() && Character->GetController()->IsLocalController())
    {
        OnRep_CurrentUsingSlot();
    }

    if (Character->HasAuthority())
    {
        if (AATGPlayerCharacter* ATGC = Cast<AATGPlayerCharacter>(Character))
        {
            ATGC->EquipWeapon(Cast<AATGWeaponBase>(TargetSlot->EquippedActor));
        }
    }
}

void UATGPlayerEquipComponent::OnRep_CurrentUsingSlot()
{
    ACharacter* Character = GetOwningPlayerCharacter();

    if (CurrentUsingSlot == EEquipmentSlotType::None)
    {
        if (AATGPlayerController* APC = Character ? Cast<AATGPlayerController>(Character->GetController()) : nullptr)
        {
            APC->WeaponInputMapping(EWeaponCategory::None);
        }
        OnEquipWeapon.Broadcast(EWeaponCategory::None);
        return;
    }

    FEquipmentSlot* TargetSlot = GetSlotByType(CurrentUsingSlot);
    if (!TargetSlot || !TargetSlot->EquippedActor)
    {
        return;
    }

    AATGWeaponBase* Weapon = Cast<AATGWeaponBase>(TargetSlot->EquippedActor);
    if (Weapon && Weapon->WeaponData)
    {
        if (AATGPlayerController* APC = Character ? Cast<AATGPlayerController>(Character->GetController()) : nullptr)
        {
            APC->WeaponInputMapping(Weapon->WeaponData->WeaponCategory);
        }
        OnEquipWeapon.Broadcast(Weapon->WeaponData->WeaponCategory);
    }
}

void UATGPlayerEquipComponent::ChangeWeaponEquip()
{
    USceneComponent* SlaveMesh = GetSlaveMesh();
    if (!SlaveMesh)
    {
        return;
    }

    FEquipmentSlot* Main1Slot = GetSlotByType(EEquipmentSlotType::MainWeapon1Slot);
    FEquipmentSlot* Main2Slot = GetSlotByType(EEquipmentSlotType::MainWeapon2Slot);
    if (!Main1Slot || !Main2Slot)
    {
        checkNoEntry();
        return;
    }

    auto AttachWeapon = [SlaveMesh](FEquipmentSlot* Slot, EEquipmentSlotType SlotType, bool bEquipped)
        {
            if (!Slot || !Slot->EquippedActor)
            {
                return;
            }

            AATGWeaponBase* Weapon = Cast<AATGWeaponBase>(Slot->EquippedActor);
            if (!Weapon || !Weapon->WeaponData)
            {
                return;
            }

            Slot->EquippedActor->AttachToComponent(
                SlaveMesh,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                Weapon->WeaponData->GetSocketName(SlotType, bEquipped)
            );
        };

    AttachWeapon(Main1Slot, EEquipmentSlotType::MainWeapon1Slot, CurrentUsingSlot == EEquipmentSlotType::MainWeapon1Slot);
    AttachWeapon(Main2Slot, EEquipmentSlotType::MainWeapon2Slot, CurrentUsingSlot == EEquipmentSlotType::MainWeapon2Slot);
}

void UATGPlayerEquipComponent::TryFire()
{
    DoFire();
}

void UATGPlayerEquipComponent::DoFire()
{
    TryWeaponFire();
}

void UATGPlayerEquipComponent::ServerDoFire_Implementation()
{
    TryWeaponFire();
}

void UATGPlayerEquipComponent::TryWeaponFire()
{
    FEquipmentSlot* TargetSlot = GetSlotByType(CurrentUsingSlot);
    if (!TargetSlot || !TargetSlot->EquippedActor)
    {
        return;
    }

    AATGWeaponBase* WeaponBase = Cast<AATGWeaponBase>(TargetSlot->EquippedActor);
    if (!WeaponBase)
    {
        return;
    }

    const float STFTime = TargetSlot->STFTime * GetReadyToFireTime();
    if (bReadyToFire || STFTime <= 0.f)
    {
        WeaponFire(WeaponBase);
        return;
    }

    if (!GetWorld()->GetTimerManager().IsTimerActive(STFTimerHandle))
    {
        GetWorld()->GetTimerManager().SetTimer(
            STFTimerHandle,
            FTimerDelegate::CreateWeakLambda(this, [this, WeaponBase]() { WeaponFire(WeaponBase); }),
            STFTime,
            false
        );
    }
}

void UATGPlayerEquipComponent::WeaponFire(AATGWeaponBase* WeaponBase)
{
    AATGRangeWeapon* RangeWeapon = Cast<AATGRangeWeapon>(WeaponBase);
    if (!RangeWeapon)
    {
        return;
    }

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
    {
        return;
    }

    RangeWeapon->Fire();

    bReadyToFire = true;
    if (!ATGCharacterInputState.WantsToAim)
    {
        GetWorld()->GetTimerManager().SetTimer(FireToMoveTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
            {
                bReadyToFire = false;
                if (AATGPlayerCharacter* ATGC = Cast<AATGPlayerCharacter>(GetOwningPlayerCharacter()))
                {
                    ATGC->RecoverMoveAnim();
                }
            }), MoveRecoveryTime, false);
    }
}

void UATGPlayerEquipComponent::ReadyToFire()
{
    FEquipmentSlot* TargetSlot = GetSlotByType(CurrentUsingSlot);
    if (!TargetSlot)
    {
        return;
    }

    const float STFTime = TargetSlot->STFTime * GetReadyToFireTime();
    if (bReadyToFire || STFTime <= 0.f)
    {
        bReadyToFire = true;
        return;
    }

    if (!GetWorld()->GetTimerManager().IsTimerActive(STFTimerHandle))
    {
        GetWorld()->GetTimerManager().SetTimer(
            STFTimerHandle,
            FTimerDelegate::CreateWeakLambda(this, [this]() { bReadyToFire = true; }),
            STFTime,
            false
        );
    }
}

void UATGPlayerEquipComponent::ReleaseAim()
{
    bReadyToFire = false;
    if (AATGPlayerCharacter* ATGC = Cast<AATGPlayerCharacter>(GetOwningPlayerCharacter()))
    {
        ATGC->RecoverMoveAnim();
    }
}

float UATGPlayerEquipComponent::GetReadyToFireTime()
{
    float Time = 0.f;
    switch (CGait)
    {
    case ECGait::Walk:
        break;
    case ECGait::Run:
        Time = 0.6f;
        break;
    case ECGait::Sprint:
        Time = 1.f;
        break;

    }
    return Time;
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

void UATGPlayerEquipComponent::InitSlot(EEquipmentSlotType InEquipmentSlotType)
{

    FEquipmentSlot EquipmentSlot;
    EquipmentSlot.SlotType = InEquipmentSlotType;
    EquipmentSlot.EquippedActor = nullptr;
    EquipmentSlots.Add(EquipmentSlot);
}

void UATGPlayerEquipComponent::SpawnRangeWeaponInSlot(FEquipmentSlot& Slot, UATGRangeWeaponData* RangeWeaponData)
{

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    FTransform SpawnTransform = OwnerCharacter->GetActorTransform();

    AATGRangeWeapon* SpawnedActor = GetWorld()->SpawnActorDeferred<AATGRangeWeapon>(
        RangeWeaponData->WeaponClass,
        SpawnTransform,
        OwnerCharacter, // Owner
        OwnerCharacter, // Instigator
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn
    );

    if (SpawnedActor)
    {
        SpawnedActor->SetReplicates(true);


        SpawnedActor->WeaponBulletData = RangeWeaponData->WeaponBulletData;
        SpawnedActor->WeaponData = RangeWeaponData;

        Slot.EquippedActor = SpawnedActor;
        Slot.STFTime = RangeWeaponData->SprinttoFireTime;
        Slot.ADSTime = RangeWeaponData->ADSTime;


        UGameplayStatics::FinishSpawningActor(SpawnedActor, SpawnTransform);

		FName AttachSocketName = RangeWeaponData->GetSocketName(Slot.SlotType, CurrentUsingSlot == Slot.SlotType);

        if (USceneComponent* AttachComp = GetSlaveMesh())
        {
            SpawnedActor->AttachToComponent(AttachComp, FAttachmentTransformRules::SnapToTargetIncludingScale, AttachSocketName);
        }
    }
}

void UATGPlayerEquipComponent::SpawnMeleeWeaponInSlot(FEquipmentSlot& Slot, UATGMeleeWeaponData* MeleeWeaponData)
{

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    FTransform SpawnTransform = OwnerCharacter->GetActorTransform();

    AATGMeleeWeapon* SpawnedActor = GetWorld()->SpawnActorDeferred<AATGMeleeWeapon>(
        MeleeWeaponData->WeaponClass,
        SpawnTransform,
        OwnerCharacter, // Owner
        OwnerCharacter, // Instigator
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn
    );

    if (SpawnedActor)
    {
        SpawnedActor->SetReplicates(true);


        SpawnedActor->WeaponData = MeleeWeaponData;

        Slot.EquippedActor = SpawnedActor;


        UGameplayStatics::FinishSpawningActor(SpawnedActor, SpawnTransform);

        FName AttachSocketName = MeleeWeaponData->GetSocketName(Slot.SlotType, CurrentUsingSlot == Slot.SlotType);

        if (USceneComponent* AttachComp = GetSlaveMesh())
        {
            SpawnedActor->AttachToComponent(AttachComp, FAttachmentTransformRules::SnapToTargetIncludingScale, AttachSocketName);
        }
    }
}

FEquipmentSlot* UATGPlayerEquipComponent::GetSlotByType(EEquipmentSlotType SlotType)
{
    return EquipmentSlots.FindByPredicate([SlotType](const FEquipmentSlot& Slot)
        {
            return Slot.SlotType == SlotType;
        });
}

EWeaponCategory UATGPlayerEquipComponent::GetCurrentEquippedWeaponCategory()
{
    FEquipmentSlot* Slot = GetSlotByType(CurrentUsingSlot);

    if (Slot && Slot->EquippedActor)
    {
        AATGWeaponBase* Weapon = Cast<AATGWeaponBase>(Slot->EquippedActor);
        if (Weapon && Weapon->WeaponData)
        {
            return Weapon->WeaponData->WeaponCategory;
        }
	}
    return EWeaponCategory::None;
}
