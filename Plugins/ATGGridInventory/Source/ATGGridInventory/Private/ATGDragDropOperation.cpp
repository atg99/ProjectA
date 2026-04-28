// Fill out your copyright notice in the Description page of Project Settings.

#include "ATGDragDropOperation.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

void UATGDragDropOperation::Dragged_Implementation(const FPointerEvent& PointerEvent)
{
	Super::Dragged_Implementation(PointerEvent);

	if (!DragController)
	{
		DragController = UGameplayStatics::GetPlayerController(this, 0);
	}

	if (!DragController)
	{
		return;
	}

	const bool bCurrentRPressed = DragController->IsInputKeyDown(EKeys::R);
	if (bCurrentRPressed && !bLastRPressed)
	{
		bIsRotated = !bIsRotated;

		if (UWidget* Ghost = DefaultDragVisual)
		{
			Ghost->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			FWidgetTransform Transform = Ghost->GetRenderTransform();
			Transform.Angle = bIsRotated ? Transform.Angle + 90.f : Transform.Angle - 90.f;
			Ghost->SetRenderTransform(Transform);
		}
	}

	bLastRPressed = bCurrentRPressed;
}
