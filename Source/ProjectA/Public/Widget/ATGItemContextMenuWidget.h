// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ATGEnum.h"
#include "ATGItemContextMenuWidget.generated.h"

/**
 * 
 */

class UATGItemData;
class UButton;
class UATGInventoryComponent;

UCLASS()
class PROJECTA_API UATGItemContextMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    // 메뉴를 열 때 호출하여 어떤 아이템인지 설정
    void InitMenu(FATGItemInfo& ItemInfo);

    UATGInventoryComponent* InvenComp;

protected:
    // UMG에서 만든 버튼과 바인딩
    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Use;

    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Drop;

private:
    UFUNCTION()
    void OnUseClicked();

    UFUNCTION()
    void OnDropClicked();

    UPROPERTY()
    FATGItemInfo CurrentItemInfo; // 현재 선택된 아이템
};
