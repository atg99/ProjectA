// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "ATGDragDropOperation.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTA_API UATGDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:

	// 드래그 중에 매 프레임 호출되는 함수
	virtual void Dragged_Implementation(const FPointerEvent& PointerEvent) override;
	
	UPROPERTY()
	TObjectPtr<APlayerController> DragController;

	bool bIsRotated = false;

	bool bLastRPressed = false;
};
