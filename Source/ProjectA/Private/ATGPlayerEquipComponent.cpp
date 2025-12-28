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
#include "Net/UnrealNetwork.h"
#include "Components/SceneComponent.h"
#include "Utils/NetworkUtil.h"
#include "ATGPlayerCharacter.h"
#include "ATGPlayerController.h"
#include "Weapon/ATGRangeWeapon.h"

// Sets default values for this component's properties
UATGPlayerEquipComponent::UATGPlayerEquipComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

    ////MainWeapon1 슬롯 추가
    //FEquipmentSlot SlotNone;
    //SlotNone.SlotType = EEquipmentSlotType::None;
    //SlotNone.EquippedActor = nullptr; // 초기엔 장비 없음
    //EquipmentSlots.Add(SlotNone);

    //FEquipmentSlot MeleeSlot;
    //MeleeSlot.SlotType = EEquipmentSlotType::MeleeWeapon;
    //MeleeSlot.EquippedActor = nullptr; // 초기엔 장비 없음
    //EquipmentSlots.Add(MeleeSlot);

    ////MainWeapon1 슬롯 추가
    //FEquipmentSlot Slot1;
    //Slot1.SlotType = EEquipmentSlotType::MainWeapon1;
    //Slot1.EquippedActor = nullptr; // 초기엔 장비 없음
    //EquipmentSlots.Add(Slot1);

    //FEquipmentSlot Slot2;
    //Slot2.SlotType = EEquipmentSlotType::MainWeapon2;
    //Slot2.EquippedActor = nullptr;
    //EquipmentSlots.Add(Slot2);

}


// Called when the game starts
void UATGPlayerEquipComponent::BeginPlay()
{
	Super::BeginPlay();

	EquipmentSlots.Empty();
    for (EEquipmentSlotType EquipmentSlotType : TEnumRange<EEquipmentSlotType>())
    {
        InitSlot(EquipmentSlotType);
    }

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

    OnRep_CurrentUsingSlot();

}

void UATGPlayerEquipComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    //애니메이션 스테이트 동기화
    DOREPLIFETIME_CONDITION(UATGPlayerEquipComponent, CurrentUsingSlot, COND_None);

    DOREPLIFETIME_CONDITION(UATGPlayerEquipComponent, EquipmentSlots, COND_None);

    //조준중인지아닌지 서버, 로컬에서 필요
    DOREPLIFETIME_CONDITION(UATGPlayerEquipComponent, ATGCharacterInputState, COND_OwnerOnly);
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
    NET_LOG("");
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

	FEquipmentSlot* Main1Slot = GetSlotByType(EEquipmentSlotType::MainWeapon1Slot);
    if (!Main1Slot)
    {
		checkNoEntry();
    }
    if (Main1Slot->EquippedActor)
    {
        Main1Slot->EquippedActor->Destroy();
        Main1Slot->EquippedActor = nullptr;
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
    }
    if (Main2Slot->EquippedActor)
    {
        Main2Slot->EquippedActor->Destroy();
        Main2Slot->EquippedActor = nullptr;
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
    NET_LOG("");
    ServerChangePlayerUsingSlot(EEquipmentSlotType::None);
}

// Called every frame
void UATGPlayerEquipComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UATGPlayerEquipComponent::TryChangePlayerUsingSlot(EEquipmentSlotType DesiredSlot)
{
    ServerChangePlayerUsingSlot(DesiredSlot);
    //클라이언트 예측
    ChangePlayerUsingSlot(DesiredSlot);
}

void UATGPlayerEquipComponent::ServerChangePlayerUsingSlot_Implementation(EEquipmentSlotType DesiredSlot)
{
    ChangePlayerUsingSlot(DesiredSlot);
}

void UATGPlayerEquipComponent::ChangePlayerUsingSlot(EEquipmentSlotType DesiredSlot)
{
    FEquipmentSlot* TargetSlot = EquipmentSlots.FindByPredicate([DesiredSlot](const FEquipmentSlot& Slot)
        {
            UE_LOG(LogTemp, Warning, TEXT("ChangePlayerUsingSlot %d"), (uint8)Slot.SlotType);
            return Slot.SlotType == DesiredSlot;
        });
    if ((TargetSlot && TargetSlot->EquippedActor) || (TargetSlot && TargetSlot->SlotType == EEquipmentSlotType::None))
    {
        NET_LOG("");
        CurrentUsingSlot = DesiredSlot;
        
        //서버에서 attach하면 동기화됨 클라에서 불필요
        ChangeWeaponEquip();

        //클라 예측이면 OnRep함수 수동호출 (인풋맵핑변경함수임) 서버와 다른 값이면 수정됨
        ACharacter* Character = GetOwningPlayerCharacter();
        if (Character)
        {
            if (Character->GetController() && Character->GetController()->IsLocalController())
            {
                OnRep_CurrentUsingSlot();
            }
        }
    }
	else if (!TargetSlot)
    {
        NET_LOG("TargetSlot Null");
    }
    else if (!TargetSlot->EquippedActor)
    {
        NET_LOG("TargetSlot EquippedActor Null");
    }
}


void UATGPlayerEquipComponent::OnRep_CurrentUsingSlot()
{
    NET_LOG("");
    EEquipmentSlotType DesiredSlot = CurrentUsingSlot;
    FEquipmentSlot* TargetSlot = EquipmentSlots.FindByPredicate([DesiredSlot](const FEquipmentSlot& Slot)
        {
            UE_LOG(LogTemp, Warning, TEXT("OnRep_CurrentUsingSlot %d"), (uint8)Slot.SlotType);
            return Slot.SlotType == DesiredSlot;
        });

    AATGWeaponBase* Weapon = Cast<AATGWeaponBase>(TargetSlot->EquippedActor);
    if (ensure(Weapon))
    {
        Weapon->WeaponData;
    }

    //총기 인풋 맵핑
    if (AATGPlayerController* APC = Cast<AATGPlayerController>(GetOwningPlayerCharacter()->GetController()))
    {
        APC->WeaponInputMapping(CurrentUsingSlot);
    }

    //서버에서 attach하면 동기화됨 클라에서 불필요
    //ChangeWeaponEquip();
}

void UATGPlayerEquipComponent::ChangeWeaponEquip()
{
    if (!GetSlaveMesh())
    {
        return;
    }
    NET_LOG("");

	FEquipmentSlot* NoneSlot = GetSlotByType(EEquipmentSlotType::None);
	FEquipmentSlot* Main1Slot = GetSlotByType(EEquipmentSlotType::MainWeapon1Slot);
	FEquipmentSlot* Main2Slot = GetSlotByType(EEquipmentSlotType::MainWeapon2Slot);
    if(!NoneSlot || !Main1Slot || !Main2Slot)
    {
        checkNoEntry();
        return;
	}

    switch (CurrentUsingSlot)
    {
    case EEquipmentSlotType::None:
    {
        if (Main1Slot->EquippedActor)
        {
            Main1Slot->EquippedActor->AttachToComponent(
                GetSlaveMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                Main1BackSocketName
            );
        }
        if (Main2Slot->EquippedActor)
        {
            Main2Slot->EquippedActor->AttachToComponent(
                GetSlaveMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                Main2BackSocketName
            );
        }
        break;
    }
    case EEquipmentSlotType::MainWeapon1Slot:
    {
        if (Main1Slot->EquippedActor)
        {
            Main1Slot->EquippedActor->AttachToComponent(
                GetSlaveMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                SniperSocketName
            );
        }
        if (Main2Slot->EquippedActor)
        {
            Main2Slot->EquippedActor->AttachToComponent(
                GetSlaveMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                Main2BackSocketName
            );
        }
        break;
    }
    case EEquipmentSlotType::MainWeapon2Slot:
    {
        if (Main1Slot->EquippedActor)
        {
            Main1Slot->EquippedActor->AttachToComponent(
                GetSlaveMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                Main1BackSocketName
            );
        }
        if (Main2Slot->EquippedActor)
        {
            Main2Slot->EquippedActor->AttachToComponent(
                GetSlaveMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                SniperSocketName
            );
        }
        break;
    }
    }
}

void UATGPlayerEquipComponent::TryFire()
{
    //Bullet Manager 에서 Parallel Simulation
    ServerDoFire();
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
    NET_LOG(TEXT("fire"));
    //현재 사용하고 있는 슬롯의 무기 유효성 검사
    EEquipmentSlotType D_CurSlotType = CurrentUsingSlot;
	FEquipmentSlot* TargetSlot = GetSlotByType(D_CurSlotType);
    if (TargetSlot && TargetSlot->EquippedActor)
    {
        if (AATGWeaponBase* WeaponBase = Cast<AATGWeaponBase>(TargetSlot->EquippedActor))
        {
            float STFTime = TargetSlot->STFTime * GetReadyToFireTime();
            float ADSTime = TargetSlot->ADSTime;
            
            //delay가 적용된 상태라면 바로 발사 아니라면 딜레이
            if (bReadyToFire || STFTime <= 0)
            {
                WeaponFire(WeaponBase);
            }
            else
            {   
                //이미 STF타이머가 돌아가고 있다면 user 광클방지
                bool bSTFTimer = GetWorld()->GetTimerManager().IsTimerActive(STFTimerHandle);
                if (!bSTFTimer)
                {
                    GetWorld()->GetTimerManager().SetTimer(STFTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this, WeaponBase]() { WeaponFire(WeaponBase); }), STFTime, false);
                }
            }
        }
    }
}

//delay 적용 플레그 업 SprintRecoveryTime 동안 발사없으면 플레그 다운
void UATGPlayerEquipComponent::WeaponFire(AATGWeaponBase* WeaponBase)
{
    AATGRangeWeapon* RangeWeapon = Cast<AATGRangeWeapon>(WeaponBase);
    //조준하는동안은 타이머 안돌게 변경 bp의 characterinputstate값
    if (RangeWeapon)
    {
        if (GetOwner()->HasAuthority())
        {
            RangeWeapon->ServerStartFire();
        }
        else
        {
            RangeWeapon->Fire();
        }
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
}

void UATGPlayerEquipComponent::ReadyToFire()
{
    EEquipmentSlotType D_CurSlotType = CurrentUsingSlot;
    FEquipmentSlot* TargetSlot = GetSlotByType(D_CurSlotType);
    float STFTime = TargetSlot->STFTime * GetReadyToFireTime();
    float ADSTime = TargetSlot->ADSTime;

    //delay가 적용된 상태라면 바로 발사 아니라면 딜레이
    if (bReadyToFire || STFTime <= 0)
    {
        bReadyToFire = true;
    }
    else
    {
        //이미 STF타이머가 돌아가고 있다면 user 광클방지
        bool bSTFTimer = GetWorld()->GetTimerManager().IsTimerActive(STFTimerHandle);
        if (!bSTFTimer)
        {
            GetWorld()->GetTimerManager().SetTimer(STFTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]() { bReadyToFire = true; }), STFTime, false);
        }
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

void UATGPlayerEquipComponent::InitSlot(EEquipmentSlotType InEquipmentSlotType)
{
	NET_LOG(FString::Printf(TEXT("Init Slot %d"), (uint8)InEquipmentSlotType));
    FEquipmentSlot EquipmentSlot;
    EquipmentSlot.SlotType = InEquipmentSlotType;
    EquipmentSlot.EquippedActor = nullptr; // 초기엔 장비 없음
    EquipmentSlots.Add(EquipmentSlot);
}

void UATGPlayerEquipComponent::SpawnRangeWeaponInSlot(FEquipmentSlot& Slot, UATGRangeWeaponData* RangeWeaponData)
{
    NET_LOG("RangeWeapon Spawn");

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

        //총알 정보 복제
        SpawnedActor->WeaponBulletData = RangeWeaponData->WeaponBulletData;
        SpawnedActor->WeaponData = RangeWeaponData;

        Slot.EquippedActor = SpawnedActor;
        Slot.STFTime = RangeWeaponData->SprinttoFireTime;
        Slot.ADSTime = RangeWeaponData->ADSTime;

        //BeginPlay 및 초기화 실행 
        UGameplayStatics::FinishSpawningActor(SpawnedActor, SpawnTransform);

        FName AttachSocketName = CurrentUsingSlot == EEquipmentSlotType::MainWeapon1Slot ? SniperSocketName : Main1BackSocketName;

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
