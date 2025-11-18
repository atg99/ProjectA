// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "ATGHUDComponent.generated.h"

//class UATGInventoryGirdWidget;
class AATGPlayerController;
class UATGHUDWidget;
class UATGContainerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryToggle, bool, bIsVisible);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnContainerToggle, UATGContainerComponent*, ContainerComponent);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTA_API UATGHUDComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UATGHUDComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UATGHUDWidget> HUDWidgetClass;

	APlayerController* GetHUDOwnerPC() const;

	bool bInvenVisible = false;

public:

	UPROPERTY(BlueprintAssignable)
	FOnInventoryToggle OnInventToggle;

	UPROPERTY(BlueprintAssignable)
	FOnContainerToggle OnContainerToggle;

	//저장(세이브)하거나 리셋(에디터 재시작) 시 보존되지 않아야 하는 변수
	UPROPERTY(Transient)
	UATGHUDWidget* HUDWidget = nullptr;

	void EnsureWidgetCreated(APlayerController* PC);
	void ToggleInventoryUI();
};
