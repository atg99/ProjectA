// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ATGEnum.h"
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

    // Display Item Name
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    EItemType ItemType = EItemType::None;

    //Category key
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    int32 ItemId;

    //Inventory Image
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    UTexture2D* Icon = nullptr;

    //World spawn item mesh
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    UStaticMesh* Mesh = nullptr;

    //Max stack at inventory grid
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    int32 MaxStack = 99;

    //Widget size at inventory grid
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Grid")
    int32 Width = 1;

    //Height size at inventory grid
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Grid")
    int32 Height = 1;
	
};
