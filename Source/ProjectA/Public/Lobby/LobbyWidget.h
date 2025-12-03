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

UCLASS()
class PROJECTA_API ULobbyWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Component, meta = (WidgetBind))
	TObjectPtr<UButton> Btn_Start;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Component, meta = (WidgetBind))
	TObjectPtr<UEditableTextBox> EditableText_Chat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Component, meta = (WidgetBind))
	TObjectPtr<UTextBlock> Text_LeftTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Component, meta = (WidgetBind))
	TObjectPtr<UTextBlock> Text_UserCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Component, meta = (WidgetBind))
	TObjectPtr<UScrollBox> ScrollBox_Chat;

	UFUNCTION()
	void HandlePressStartBtn();

	UFUNCTION()
	void HandleTextCommit(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void ProcessOnChange(const FText& Text);

	UFUNCTION(BlueprintCallable)
	void UpdateLeftTime(int32 InLeftTime);
};
