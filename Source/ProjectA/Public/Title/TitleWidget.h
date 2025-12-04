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
	TObjectPtr<UButton> StartServerButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Widget, meta = (BindWidget))
	TObjectPtr<UButton> ConnectButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Widget, meta = (BindWidget))
	TObjectPtr<UEditableTextBox> UserID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Widget, meta = (BindWidget))
	TObjectPtr<UEditableTextBox> Password;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Widget, meta = (BindWidget))
	TObjectPtr<UEditableTextBox> ServerIP;

	UFUNCTION()
	void StartServer();

	UFUNCTION()
	void Connect();

	void SaveData();
};
