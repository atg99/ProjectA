// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyWidget.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void ULobbyWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Btn_Start)
	{
		Btn_Start->OnClicked.AddDynamic(this, &ULobbyWidget::HandlePressStartBtn);
	}

	if (EditableText_Chat)
	{
		EditableText_Chat->OnTextCommitted.AddDynamic(this, &ULobbyWidget::HandleTextCommit);
		EditableText_Chat->OnTextChanged.AddDynamic(this, &ULobbyWidget::ProcessOnChange);
	}
}

void ULobbyWidget::HandlePressStartBtn()
{
	GetWorld()->ServerTravel(TEXT("DevLevel"));
}

void ULobbyWidget::HandleTextCommit(const FText& Text, ETextCommit::Type CommitMethod)
{
}

void ULobbyWidget::ProcessOnChange(const FText& Text)
{
}

void ULobbyWidget::UpdateLeftTime(int32 InLeftTime)
{
	if (InLeftTime)
	{
		FString Message = FString::Printf(TEXT("%dÃÊ ³²À½"), InLeftTime);
		//Text_LeftTime->SetText(M);
	}
}


