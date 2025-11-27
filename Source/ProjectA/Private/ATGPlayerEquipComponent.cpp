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

void UATGPlayerEquipComponent::ServerChangePlayerUsingSlot_Implementation(EEquipmentSlotType TryUsingSlot)
{
    UE_LOG(LogTemp, Log, TEXT("ServerChangePlayerUsingSlot_Implementation"));
    CurrentUsingSlot = TryUsingSlot;
    ChangeWeaponEquip();
}

void UATGPlayerEquipComponent::HandleFirstMainWeaponChanged(FInventoryEntry InFirstMainWeapon)
{
    if (GetOwner()->HasAuthority())
    {
        return;
    }
    //null이 되어서 들어왔든, 다른 아이템으로 바뀌어서 들어왔든 기존 액터는 지워야 함
    if (EquipmentSlots[0].EquippedActor)
    {
        EquipmentSlots[0].EquippedActor->Destroy();
        EquipmentSlots[0].EquippedActor = nullptr;
    }

    // 1. 유효성 검사 (ItemData 및 WeaponData 확인)
    UATGItemData* ItemData = InFirstMainWeapon.Item.Get();
    if (!ItemData) return;

    UATGWeaponData* WeaponData = Cast<UATGWeaponData>(ItemData);
    if (!WeaponData || !WeaponData->WeaponClass) return;

    // 2. 오너 캐릭터 및 월드 가져오기
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    UWorld* World = GetWorld();
    if (!World) return;

    // 3. 스폰 트랜스폼 준비 (Attach 할 것이므로 위치는 0,0,0이어도 무관하지만 기본값 설정)
    FTransform SpawnTransform = OwnerCharacter->GetActorTransform();

    // ----------------------------------------------------------------
    // 4. [중요] SpawnActorDeferred 호출
    // ----------------------------------------------------------------
    // AATGWeaponBase는 실제 무기 액터 클래스로 변경하세요 (예: AMyWeaponActor)
    // 템플릿(< >) 안에 실제 무기 클래스 타입을 넣어야 멤버 변수에 접근 가능합니다.
    AActor* SpawnedActor = World->SpawnActorDeferred<AActor>(
        WeaponData->WeaponClass,
        SpawnTransform,
        OwnerCharacter, // Owner 설정
        OwnerCharacter, // Instigator 설정
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn
    );

    // 5. 초기값 대입 및 스폰 마무리
    if (SpawnedActor)
    {
        // 실제 무기 클래스로 캐스팅하여 데이터 주입
        // (예시: AATGWeaponBase* NewWeapon = Cast<AATGWeaponBase>(SpawnedActor);)
        /*
        if (NewWeapon)
        {
             NewWeapon->Damage = WeaponData->BaseDamage; // 예시: 초기값 대입
             NewWeapon->WeaponID = WeaponData->ID;       // 예시: 초기값 대입
        }
        */
        SpawnedActor->SetReplicates(true);
        //슬롯배열에 추가
        EquipmentSlots[0].EquippedActor = SpawnedActor;
        //BeginPlay 및 초기화 실행 
        UGameplayStatics::FinishSpawningActor(SpawnedActor, SpawnTransform);

        FName AttachSocketName = CurrentUsingSlot == EEquipmentSlotType::MainWeapon1 ? SniperSocketName : Main1BackSocketName;

        SpawnedActor->AttachToComponent(
            OwnerCharacter->GetMesh(),
            FAttachmentTransformRules::SnapToTargetIncludingScale, // 위치,회전,크기 모두 소켓에 맞춤
            AttachSocketName
        );
    }
}

void UATGPlayerEquipComponent::HandleSecondMainWeaponChanged(FInventoryEntry InSecondMainWeapon)
{
    // null이 되어서 들어왔든, 다른 아이템으로 바뀌어서 들어왔든 기존 액터는 지워야 함
    if (GetOwner()->HasAuthority())
    {
        return;
    }

    if (EquipmentSlots[1].EquippedActor)
    {
        EquipmentSlots[1].EquippedActor->Destroy();
        EquipmentSlots[1].EquippedActor = nullptr;
    }

    // 1. 유효성 검사 (ItemData 및 WeaponData 확인)
    UATGItemData* ItemData = InSecondMainWeapon.Item.Get();
    if (!ItemData) return;

    UATGWeaponData* WeaponData = Cast<UATGWeaponData>(ItemData);
    if (!WeaponData || !WeaponData->WeaponClass) return;

    // 2. 오너 캐릭터 및 월드 가져오기
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    UWorld* World = GetWorld();
    if (!World) return;

    // 3. 스폰 트랜스폼 준비 (Attach 할 것이므로 위치는 0,0,0이어도 무관하지만 기본값 설정)
    FTransform SpawnTransform = OwnerCharacter->GetActorTransform();

    // ----------------------------------------------------------------
    // 4. [중요] SpawnActorDeferred 호출
    // ----------------------------------------------------------------
    // AATGWeaponBase는 실제 무기 액터 클래스로 변경하세요 (예: AMyWeaponActor)
    // 템플릿(< >) 안에 실제 무기 클래스 타입을 넣어야 멤버 변수에 접근 가능합니다.
    AActor* SpawnedActor = World->SpawnActorDeferred<AActor>(
        WeaponData->WeaponClass,
        SpawnTransform,
        OwnerCharacter, // Owner 설정
        OwnerCharacter, // Instigator 설정
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn
    );

    // 5. 초기값 대입 및 스폰 마무리
    if (SpawnedActor)
    {
        // 실제 무기 클래스로 캐스팅하여 데이터 주입
        // (예시: AATGWeaponBase* NewWeapon = Cast<AATGWeaponBase>(SpawnedActor);)
        /*
        if (NewWeapon)
        {
             NewWeapon->Damage = WeaponData->BaseDamage; // 예시: 초기값 대입
             NewWeapon->WeaponID = WeaponData->ID;       // 예시: 초기값 대입
        }
        */
        SpawnedActor->SetReplicates(true);
        //슬롯배열에 추가
        EquipmentSlots[1].EquippedActor = SpawnedActor;
        // BeginPlay 및 초기화 실행 (이 시점에 액터가 세상에 완전히 태어남)
        UGameplayStatics::FinishSpawningActor(SpawnedActor, SpawnTransform);

        FName AttachSocketName = CurrentUsingSlot == EEquipmentSlotType::MainWeapon2 ? SniperSocketName : Main2BackSocketName;

        SpawnedActor->AttachToComponent(
            OwnerCharacter->GetMesh(),
            FAttachmentTransformRules::SnapToTargetIncludingScale, // 위치,회전,크기 모두 소켓에 맞춤
            AttachSocketName
        );
    }
}

// Called every frame
void UATGPlayerEquipComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UATGPlayerEquipComponent::OnRep_CurrentUsingSlot()
{
    UE_LOG(LogTemp, Log, TEXT("UATGPlayerEquipComponent::OnRep_CurrentUsingSlot"));
}

void UATGPlayerEquipComponent::ChangeWeaponEquip()
{
    if (!GetOwningPlayerCharacter())
    {
        return;
    }


    switch (CurrentUsingSlot)
    {
    case EEquipmentSlotType::MainWeapon1:
    {
        if (EquipmentSlots[0].EquippedActor)
        {
            EquipmentSlots[0].EquippedActor->AttachToComponent(
                GetOwningPlayerCharacter()->GetMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                SniperSocketName
            );
        }
        if (EquipmentSlots[1].EquippedActor)
        {
            EquipmentSlots[1].EquippedActor->AttachToComponent(
                GetOwningPlayerCharacter()->GetMesh(),
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
                GetOwningPlayerCharacter()->GetMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                Main1BackSocketName
            );
        }
        if (EquipmentSlots[1].EquippedActor)
        {
            EquipmentSlots[1].EquippedActor->AttachToComponent(
                GetOwningPlayerCharacter()->GetMesh(),
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

