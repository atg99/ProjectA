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

    // Asset Name
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    UTexture2D* Icon = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    UStaticMesh* Mesh = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    int32 MaxStack = 99;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Grid")
    int32 Width = 1;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Grid")
    int32 Height = 1;
	
};
