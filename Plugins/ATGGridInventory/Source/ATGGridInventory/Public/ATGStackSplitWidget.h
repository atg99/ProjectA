// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ATGStackSplitWidget.generated.h"

class UButton;
class USlider;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSplitConfirmed, int32);

UCLASS()
class ATGGRIDINVENTORY_API UATGStackSplitWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> SplitSlider = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TxtCount = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BtnConfirm = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BtnCancel = nullptr;

	FOnSplitConfirmed OnSplitConfirmed;

	int32 MaxCount = 1;

	virtual void NativeConstruct() override;

	void InitSplit(int32 InMaxCount);

	UFUNCTION()
	void OnSliderChanged(float Value);

	UFUNCTION()
	void OnConfirmClicked();

	UFUNCTION()
	void OnCancelClicked();

	UFUNCTION(BlueprintCallable)
	void RemoveSplitWidget();
};
