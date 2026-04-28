// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ATGInventoryEnums.h"
#include "Data/ATGItemData.h"
#include "ATGInventoryInterface.generated.h"

// Moved from ProjectA ATGEnum.h — only uses plugin-owned types
USTRUCT(BlueprintType)
struct ATGGRIDINVENTORY_API FInteractionData
{
	GENERATED_BODY()

	FInteractionData()
		: InteractedActor(nullptr)
		, InteractedComponent(nullptr)
		, InteractionType(EInteractionType::None)
		, ItemQty(0)
	{}

	UPROPERTY(BlueprintReadWrite)
	AActor* InteractingActor = nullptr;

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

class UATGInventoryItemWidget;

USTRUCT(BlueprintType)
struct ATGGRIDINVENTORY_API FATGItemInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UATGInventoryItemWidget> ItemWidget = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UATGItemData> ItemDef;

	UPROPERTY(BlueprintReadOnly)
	int32 EntryId = -1;

	UPROPERTY(BlueprintReadOnly)
	int32 Quantity = -1;

	UPROPERTY(BlueprintReadOnly)
	bool bIsRotated = false;
};

// Moved from ProjectA ATGInterface.h
UINTERFACE(MinimalAPI, NotBlueprintable)
class UATGInterface : public UInterface
{
	GENERATED_BODY()
};

class ATGGRIDINVENTORY_API IATGInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void PlayerInteract(FInteractionData& InteractionData) = 0;
};
