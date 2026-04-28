// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ATGInventoryEnums.h"
#include "GameplayTagContainer.h"
#include "ATGItemData.generated.h"

UCLASS(BlueprintType)
class ATGGRIDINVENTORY_API UATGItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	static const FPrimaryAssetType ATGItemData;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EItemType ItemType = EItemType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	int32 ItemId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	UTexture2D* Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	UStaticMesh* Mesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	int32 MaxStack = 99;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	int32 SellPrice = 100;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Grid")
	int32 Width = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Grid")
	int32 Height = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	FGameplayTagContainer OwnedTags;
};
