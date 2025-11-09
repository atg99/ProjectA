// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGStackSplitWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UATGStackSplitWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (SplitSlider)
    {
        SplitSlider->OnValueChanged.AddDynamic(this, &UATGStackSplitWidget::OnSliderChanged);
    }
    if (BtnConfirm)
    {
        BtnConfirm->OnClicked.AddDynamic(this, &UATGStackSplitWidget::OnConfirmClicked);
    }
    if (BtnCancel)
    {
        BtnCancel->OnClicked.AddDynamic(this, &UATGStackSplitWidget::OnCancelClicked);
    }
}

void UATGStackSplitWidget::InitSplit(int32 InMaxCount)
{
    MaxCount = FMath::Max(1, InMaxCount);
    if (SplitSlider)
    {
        SplitSlider->SetMinValue(1);
        SplitSlider->SetMaxValue(MaxCount - 1);
        SplitSlider->SetValue(FMath::Max(1, MaxCount / 2));
    }
    OnSliderChanged(SplitSlider->GetValue());
}

void UATGStackSplitWidget::OnSliderChanged(float Value)
{
    if (TxtCount)
    {
        TxtCount->SetText(FText::AsNumber(FMath::RoundToInt(Value)));
    }
}

void UATGStackSplitWidget::OnConfirmClicked()
{
    int32 SplitNum = FMath::RoundToInt(SplitSlider->GetValue());
    OnSplitConfirmed.Broadcast(SplitNum);
    RemoveSplitWidget();
}

void UATGStackSplitWidget::OnCancelClicked()
{
    RemoveSplitWidget();
}

void UATGStackSplitWidget::RemoveSplitWidget()
{
    SetVisibility(ESlateVisibility::Collapsed);
}
