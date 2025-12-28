// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ATGEnum.generated.h"
/**
 * 
 */

UENUM(BlueprintType)
enum class EItemType : uint8
{
	None		=0		UMETA(DisplayName = "None"),
	Equipment	=10		UMETA(DisplayName = "Equipment"),
	Consumables	=20		UMETA(DisplayName = "Consumables"),
	Materials 	=30		UMETA(DisplayName = "Materials"),
};

UENUM(BlueprintType)
enum class EEquipmentType : uint8
{
	None	=0		UMETA(DisplayName = "None"),
	Weapon	=10		UMETA(DisplayName = "Weapon"),
	Armor	=20		UMETA(DisplayName = "Armor"),
};

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None		= 0		UMETA(DisplayName = "None"),
	MainWeapon  = 10	UMETA(DisplayName = "MainWeapon"),
	SubWeapon   = 20	UMETA(DisplayName = "SubWeapon"),
};

UENUM(BlueprintType)
enum class EEquipmentSlotType : uint8
{
	None		= 0			UMETA(DisplayName = "None"),
	MainWeapon1Slot	= 1		UMETA(DisplayName = "MainWeapon1Slot"),
	MainWeapon2Slot	= 2		UMETA(DisplayName = "MainWeapon2Slot"),
};
ENUM_RANGE_BY_FIRST_AND_LAST(EEquipmentSlotType, EEquipmentSlotType::None, EEquipmentSlotType::MainWeapon2Slot);

UENUM(BlueprintType)
enum class EInteractionType : uint8
{
	None			UMETA(DisplayName = "None"),
	PickUpItem		UMETA(DisplayName = "PickUpItem"),
	ItemGridBox		UMETA(DisplayName = "OpenItemGrid"),
	Equipment		UMETA(DisplayName = "Equipment"),
};


UENUM(BlueprintType)
enum class EMonsterState : uint8
{
	None = 0	UMETA(DisplayName = "None"),
	Normal = 10	UMETA(DisplayName = "Normal"),
	Chase = 20	UMETA(DisplayName = "Chase"),
	Battle = 30	UMETA(DisplayName = "Battle"),
	Death = 40	UMETA(DisplayName = "Death"),
};

UENUM(BlueprintType)
enum class ECGait : uint8
{
	Walk = 0	UMETA(DisplayName = "Walk"),
	Run = 1		UMETA(DisplayName = "Run"),
	Sprint = 2	UMETA(DisplayName = "Sprint"),
};

UENUM(BlueprintType)
enum class EPlayerAbilityInputID : uint8
{
	None,

	// GAS 필수 예약어 Confirm
	Confirm,

	// GAS 필수 예약어 Cancel
	Cancel,

	MeleeAttack,
};

class UATGItemData;

USTRUCT(BlueprintType)
struct FInteractionData
{
	GENERATED_BODY()

	FInteractionData()
		: InteractedActor(nullptr)
		, InteractedComponent(nullptr)
		, InteractionType(EInteractionType::None)
		, ItemQty(0)
	{
	}

	UPROPERTY(BlueprintReadWrite)
	AActor* InteractingActor;

	UPROPERTY(BlueprintReadWrite)
	AActor* InteractedActor;

	UPROPERTY(BlueprintReadWrite)
	UActorComponent* InteractedComponent;

	UPROPERTY(BlueprintReadWrite)
	EInteractionType InteractionType;

	UPROPERTY(BlueprintReadWrite)
	TSoftObjectPtr<UATGItemData> ItemDef;

	UPROPERTY(BlueprintReadWrite)
	int32 ItemQty;
};

USTRUCT(BlueprintType)
struct FEquipmentSlot
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EEquipmentSlotType SlotType = EEquipmentSlotType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AActor> EquippedActor = nullptr;

	UPROPERTY(BlueprintReadWrite)
	float STFTime = 0.5;

	UPROPERTY(BlueprintReadWrite)
	float ADSTime = 0.2;
};

//날아가고 있는 총알 정보
USTRUCT(BlueprintType)
struct FBullet
{
	GENERATED_BODY()
public:
	//시작 위치
	UPROPERTY()
	FVector StartLocation;
	//위치
	UPROPERTY()
	FVector Location;
	//속도 방향 + 속력
	UPROPERTY()
	FVector Velocity;
	// 중력 영향도 1.0 ~ 0.0
	UPROPERTY()
	float GravityScale;
	// 항력 계수 0.0이면 저항 없음, 값이 클수록 빨리 느려짐
	UPROPERTY()
	float DragCoefficient;

	UPROPERTY()
	TArray<AActor*> PierceActors;
	UPROPERTY()
	AActor* BulletOwner;
	UPROPERTY()
	TArray<AActor*> IgnoreActors;
	UPROPERTY()
	float LifeTime = 0;
};

USTRUCT(BlueprintType)
struct FBulletHitResult
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FBullet Bullet;
	UPROPERTY()
	FVector HitLocation;
	UPROPERTY()
	FHitResult Result;
};

//무기의 총알 정보
USTRUCT(BlueprintType)
struct FWeaponBulletData
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackPower = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GravityScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DragCoefficient = 1.f;

	//탄속
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed = 500.f;

	//저지력
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ImpulseScale = 10000;
};

//Character Input State
USTRUCT(BlueprintType)
struct FATGCharacterInputState
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite)
	bool WantsToAim;
};