// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ATGInventoryEnums.h"
#include "ATGInventoryInterface.h"  // FInteractionData, IATGInterface (moved to plugin)
#include "ATGEnum.generated.h"

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

// FInteractionData moved to ATGInventoryInterface.h (ATGGridInventory plugin)
class UATGItemData;

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

//���ư��� �ִ� �Ѿ� ����
USTRUCT(BlueprintType)
struct FBullet
{
	GENERATED_BODY()
public:
	//���� ��ġ
	UPROPERTY()
	FVector StartLocation;
	//��ġ
	UPROPERTY()
	FVector Location;
	//�ӵ� ���� + �ӷ�
	UPROPERTY()
	FVector Velocity;
	// �߷� ���⵵ 1.0 ~ 0.0
	UPROPERTY()
	float GravityScale;
	// �׷� ���?0.0�̸� ���� ����, ���� Ŭ���� ���� ������
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

//������ �Ѿ� ����
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

	//ź��
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed = 500.f;

	//������
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

// FATGItemInfo moved to ATGInventoryInterface.h (ATGGridInventory plugin)