// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ATGStackSplitWidget.generated.h"

/**
 * 
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSplitConfirmed, int32);


UCLASS()
class PROJECTA_API UATGStackSplitWidget : public UUserWidget
{
	GENERATED_BODY()
	

public:
    UPROPERTY(meta = (BindWidget)) class USlider* SplitSlider;
    UPROPERTY(meta = (BindWidget)) class UTextBlock* TxtCount;
    UPROPERTY(meta = (BindWidget)) class UButton* BtnConfirm;
    UPROPERTY(meta = (BindWidget)) class UButton* BtnCancel;

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
