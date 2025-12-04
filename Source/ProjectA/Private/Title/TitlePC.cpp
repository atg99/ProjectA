// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/TitlePC.h"
#include "Title/TitleWidget.h"

void ATitlePC::BeginPlay()
{
	Super::BeginPlay();

	if (TitleWidgetClass)
	{
		//new Class
		TitleWidgetObject = CreateWidget<UTitleWidget>(this, TitleWidgetClass);
		if (TitleWidgetObject)
		{
			TitleWidgetObject->AddToViewport();
		}
	}

	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());
}

void ATitlePC::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}
