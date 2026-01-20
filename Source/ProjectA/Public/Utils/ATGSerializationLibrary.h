// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InventoryTypes.h"
#include "ATGSerializationLibrary.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTA_API UATGSerializationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	static TSharedPtr<FJsonObject> SerializeActorToJson(AActor* Actor);

	static void DeserializeJsonToActor(AActor* Actor, TSharedPtr<FJsonObject> JsonObj);

	// FInventoryGrid -> Json String 저장
	UFUNCTION(BlueprintCallable, Category = "SaveLoad")
	static FString ConvertGridToJson(const FInventoryGrid& Grid);

	// Json String -> FInventoryGrid 로드
	UFUNCTION(BlueprintCallable, Category = "SaveLoad")
	static bool ConvertJsonToGrid(const FString& JsonString, FInventoryGrid& OutGrid);
};
