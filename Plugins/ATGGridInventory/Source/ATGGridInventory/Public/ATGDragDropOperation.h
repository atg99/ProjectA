// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "ATGDragDropOperation.generated.h"

UCLASS()
class ATGGRIDINVENTORY_API UATGDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	virtual void Dragged_Implementation(const FPointerEvent& PointerEvent) override;

	UPROPERTY()
	TObjectPtr<APlayerController> DragController;

	bool bIsRotated = false;
	bool bLastRPressed = false;
};
