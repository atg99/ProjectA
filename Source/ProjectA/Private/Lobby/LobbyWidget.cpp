// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyWidget.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Lobby/LobbyGameState.h"
#include "Lobby/LobbyPC.h"
#include "Components/RichTextBlock.h"
#include "Utils/NetworkUtil.h"
#include "Title/NetworkGameInstanceSubsystem.h"
#include "Components/RichTextBlockImageDecorator.h"

void ULobbyWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	//if (Btn_Start)
	//{
	//	Btn_Start->OnClicked.AddDynamic(this, &ULobbyWidget::HandlePressStartBtn);
	//	Btn_Start->SetVisibility(ESlateVisibility::Hidden);
	//}

	if (EditableText_Chat)
	{
		EditableText_Chat->OnTextCommitted.AddDynamic(this, &ULobbyWidget::HandleTextCommit);
		EditableText_Chat->OnTextChanged.AddDynamic(this, &ULobbyWidget::ProcessOnChange);
	}

	//deprecated
	//ALobbyGameState* LobbyGS = GetWorld()->GetGameState() ? Cast<ALobbyGameState>(GetWorld()->GetGameState()) :nullptr;
	//if (LobbyGS)
	//{
	//	UpdatePlayerNum(LobbyGS->PlayerArray.Num());
	//	LobbyGS->OnLeftTime.AddDynamic(this, &ULobbyWidget::UpdateLeftTime);
	//	LobbyGS->OnPlayerNum.AddDynamic(this, &ULobbyWidget::UpdatePlayerNum);
	//}
}

//void ULobbyWidget::HandlePressStartBtn()
//{
//	GetWorld()->ServerTravel(TEXT("DevLevel"));
//}

void ULobbyWidget::HandleTextCommit(const FText& Text, ETextCommit::Type CommitMethod)
{
	switch (CommitMethod)
	{
	case ETextCommit::OnEnter:
	{
		UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld());
		if (GI)
		{
			bool bHost = true;

			FString UserLogo = bHost ? FString::Printf(TEXT("<img id=\"Logo.Host\"/>")) : FString::Printf(TEXT("<img id=\"Logo.Client\"/>"));

			FString UserID = "Unknown";
			UNetworkGameInstanceSubsystem* MySubsystem = GI->GetSubsystem<UNetworkGameInstanceSubsystem>();
			if (MySubsystem && !MySubsystem->UserID.IsEmpty())
			{
				UserID = FString::Printf(TEXT("%s"), *MySubsystem->UserID);
			}
			FString Role = bHost ? FString::Printf(TEXT("<Rich.Host>%s</>"), *UserID) : FString::Printf(TEXT("<Rich.Client>%s </>"), *UserID);

			FString Message = FString::Printf(TEXT("%s%s : %s"),*UserLogo, *Role, *Text.ToString());

			MySubsystem->SendChatMessage(Message);

			EditableText_Chat->SetText(FText::FromString(TEXT("")));
		}
	}
	break;
	case ETextCommit::OnCleared:
		break;
	}
}

void ULobbyWidget::ProcessOnChange(const FText& Text)
{
}

void ULobbyWidget::UpdateLeftTime(int32 InLeftTime)
{
	if (InLeftTime >= 0)
	{
		FString Message = FString::Printf(TEXT("%d초 남음"), InLeftTime);
		Text_LeftTime->SetText(FText::FromString(Message));
	}
}

void ULobbyWidget::UpdatePlayerNum(int32 InPlayerNum)
{
	FString Message = FString::Printf(TEXT("%d명 접속"), InPlayerNum);
	Text_UserCount->SetText(FText::FromString(Message));
}

void ULobbyWidget::AddMessage(const FText& Message)
{
	//NET_LOG(TEXT("Add"));
	if (ScrollBox_Chat)
	{
		
		URichTextBlock* NewMessageBlock = NewObject<URichTextBlock>(ScrollBox_Chat);
		if (NewMessageBlock)
		{
			NewMessageBlock->SetText(Message);
			NewMessageBlock->SetAutoWrapText(true);
			NewMessageBlock->SetWrapTextAt(ScrollBox_Chat->GetCachedGeometry().GetLocalSize().X);
			NewMessageBlock->SetWrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping);
			NewMessageBlock->SetVisibility(ESlateVisibility::Visible);

			if (ChatStyleSet)
			{
				//NET_LOG(TEXT("ChatStyleSet"));
				NewMessageBlock->SetTextStyleSet(ChatStyleSet);
			}
			if (!RichTextImageDecorators.IsEmpty())
			{
				//NET_LOG(TEXT("RichTextImageDecorator"));

				NewMessageBlock->SetDecorators(RichTextImageDecorators);
			}

			ScrollBox_Chat->AddChild(NewMessageBlock);
			ScrollBox_Chat->ScrollToEnd();
		}
	}
}

//void ULobbyWidget::ShowStartBtn()
//{
//	if (Btn_Start)
//	{
//		Btn_Start->SetVisibility(ESlateVisibility::Visible);
//	}
//}


