// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ATGItemData.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "ATGConsumableItemData.generated.h"

class UAnimMontage;

USTRUCT(BlueprintType)
struct FItemEffectInfo
{
    GENERATED_BODY()

    // 적용할 GE 템플릿 (예: GE_Instant_Add, GE_Duration_Add 등)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TSubclassOf<UGameplayEffect> EffectClass;

    // 어떤 데이터를 넘길 것인가? (예: Data.Effect.Amount)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayTag DataTag;

    // 실제 수치 
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float Magnitude;
};

/**
 * 
 */
UCLASS()
class PROJECTA_API UATGConsumableItemData : public UATGItemData
{
	GENERATED_BODY()

public:
	UATGConsumableItemData();

    // 아이템을 "사용"하는 행동
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TSubclassOf<UGameplayAbility> ActiveAbility;

    // 사용했을 때 적용될 효과
    // 어빌리티가 실행될 때 이 Effect 클래스를 가져다 캐릭터에게 적용
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TArray<FItemEffectInfo> ItemEffects;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    UAnimMontage* ConsumMontage;
};
