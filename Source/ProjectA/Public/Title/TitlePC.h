// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TitlePC.generated.h"

/**
 * 
 */
class UTitleWidget;

UCLASS()
class PROJECTA_API ATitlePC : public APlayerController
{
	GENERATED_BODY()
	
public:
	void BeginPlay() override;

	virtual void PostInitializeComponents() override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UMG)
	TSubclassOf<UTitleWidget> TitleWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = UMG)
	TObjectPtr<UTitleWidget> TitleWidgetObject;

};
