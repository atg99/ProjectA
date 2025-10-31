// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "ATGHUDComponent.generated.h"

class UATGInventoryGirdWidget;
class AATGPlayerController;

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
	TSubclassOf<UATGInventoryGirdWidget> InventoryWidgetClass;

	APlayerController* GetHUDOwnerPC() const;

public:
	//����(���̺�)�ϰų� ����(������ �����) �� �������� �ʾƾ� �ϴ� ����
	UPROPERTY(Transient)
	UATGInventoryGirdWidget* InventoryWidget = nullptr;

	void EnsureWidgetCreated(APlayerController* PC);
	void ToggleInventoryUI();
};
