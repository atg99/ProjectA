// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TitleWidget.generated.h"

/**
 * 
 */
class UButton;
class UEditableTextBox;
UCLASS()
class PROJECTA_API UTitleWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	virtual void NativeConstruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Widget, meta = (BindWidget))
	TObjectPtr<UButton> RegisterButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Widget, meta = (BindWidget))
	TObjectPtr<UButton> ConnectButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Widget, meta = (BindWidget))
	TObjectPtr<UEditableTextBox> UserID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Widget, meta = (BindWidget))
	TObjectPtr<UEditableTextBox> Password;

	UFUNCTION()
	void StartServer();

	UFUNCTION(BlueprintCallable)
	void LoginAndConnect();

	UFUNCTION(BlueprintCallable)
	void RegistServer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backend")
	FString BackendIP = TEXT("127.0.0.1");
protected:
	void SaveData();
};
