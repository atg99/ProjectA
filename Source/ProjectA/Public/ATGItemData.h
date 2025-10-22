// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ATGItemData.generated.h"

/**
 * 
 */
//�������Ʈ ���� 
UCLASS(BlueprintType)
class PROJECTA_API UATGItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

    // �����Ϳ��� ���� �̸�(���ϸ�) ��ü�� ���� ID ������ ���ص� ��
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    UTexture2D* Icon = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    int32 MaxStack = 99;
	
};
