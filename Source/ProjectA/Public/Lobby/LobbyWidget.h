// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyWidget.generated.h"

/**
 * 
 */
class UButton;
class UTextBlock;
class UScrollBox;
class UEditableTextBox;
class URichTextBlockImageDecorator;
class URichTextBlockDecorator;

UCLASS()
class PROJECTA_API ULobbyWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Component, meta = (BindWidget))
	TObjectPtr<UButton> Btn_Start;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Component, meta = (BindWidget))
	TObjectPtr<UEditableTextBox> EditableText_Chat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Component, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_LeftTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Component, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_UserCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Component, meta = (BindWidget))
	TObjectPtr<UScrollBox> ScrollBox_Chat;

	UFUNCTION()
	void HandlePressStartBtn();

	UFUNCTION()
	void HandleTextCommit(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void ProcessOnChange(const FText& Text);

	UFUNCTION(BlueprintCallable)
	void UpdateLeftTime(int32 InLeftTime);

	UFUNCTION(BlueprintCallable)
	void UpdatePlayerNum(int32 InPlayerNum);

	UFUNCTION(BlueprintCallable)
	void AddMessage(const FText& Message);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UDataTable> ChatStyleSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TArray<TSubclassOf<URichTextBlockDecorator>> RichTextImageDecorators;

	void ShowStartBtn();
};
