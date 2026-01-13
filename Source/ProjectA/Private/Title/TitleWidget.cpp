// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/TitleWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Title/NetworkGameInstanceSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UTitleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//ServerIP = Cast<UEditableTextBox>(GetWidgetFromName(TEXT("ServerIP")));

	//StartServerButton->OnClicked.AddDynamic(this, &UTitleWidget::StartServer);
	ConnectButton->OnClicked.AddDynamic(this, &UTitleWidget::Connect);
}

//사용안함
void UTitleWidget::StartServer()
{
	//웹api에서 로그인함 로비서버 listen서버 아님
	SaveData();
	UGameplayStatics::OpenLevel(GetWorld(), TEXT("LobbyMap"), true, TEXT("listen"));
}

void UTitleWidget::Connect()
{
	SaveData();
	//UGameplayStatics::OpenLevel(GetWorld(), FName(ServerIP->GetText().ToString()), true, TEXT("Option"));
}

void UTitleWidget::SaveData()
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld());
	if (GI)
	{
		UNetworkGameInstanceSubsystem* MySubsystem = GI->GetSubsystem<UNetworkGameInstanceSubsystem>();
		MySubsystem->UserID = UserID->GetText().ToString();
		MySubsystem->Password = Password->GetText().ToString();
	}
}