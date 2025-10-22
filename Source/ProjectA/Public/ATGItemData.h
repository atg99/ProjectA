// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ATGItemData.generated.h"

/**
 * 
 */
//블루프린트 변수 
UCLASS(BlueprintType)
class PROJECTA_API UATGItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

    // 에디터에서 에셋 이름(파일명) 자체가 고유 ID 역할을 겸해도 됨
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    UTexture2D* Icon = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    int32 MaxStack = 99;
	
};
