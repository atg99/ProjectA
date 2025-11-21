// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGDragDropOperation.h"
#include <Kismet/GameplayStatics.h>
#include "UMG.h"

void UATGDragDropOperation::Dragged_Implementation(const FPointerEvent& PointerEvent)
{
	Super::Dragged_Implementation(PointerEvent);

	if (!DragController)
	{
		DragController = UGameplayStatics::GetPlayerController(this, 0);
	}

	bool bCurrentRPressed = DragController->IsInputKeyDown(EKeys::R);
	if (bCurrentRPressed && !bLastRPressed)
	{
		bIsRotated = !bIsRotated;

		if (UWidget* Ghost = DefaultDragVisual)
		{
			//90도 시각 회전
			Ghost->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			FWidgetTransform T = Ghost->GetRenderTransform();

			T.Angle = bIsRotated ? T.Angle += 90.f : T.Angle -= 90.f;

			Ghost->SetRenderTransform(T);
		}

		
		UE_LOG(LogTemp, Log, TEXT("Drag Rotation Toggled: %s"), bIsRotated ? TEXT("True") : TEXT("False"));
	}

	bLastRPressed = bCurrentRPressed;
}