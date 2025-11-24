// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Public/InventoryTypes.h"
#include "ATGInventoryOwnerInterface.h"
#include "ATGEquipmentComponent.generated.h"

struct FInventoryEntry;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentEvent, int32, EntryId);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTA_API UATGEquipmentComponent : public UActorComponent, public IATGInventoryOwnerInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UATGEquipmentComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
public:

	UPROPERTY(Replicated, BlueprintReadOnly)
	FInventoryEntry MainWeapon1;

	UPROPERTY(Replicated, BlueprintReadOnly)
	FInventoryEntry MainWeapon2;

	TArray<FInventoryEntry> Entries;

public:

	// client UI
	UPROPERTY(BlueprintAssignable)
	FOnEquipmentEvent OnEquipmentAdded;

	UPROPERTY(BlueprintAssignable)
	FOnEquipmentEvent OnEquipmentRemoved;

	UPROPERTY(BlueprintAssignable)
	FOnEquipmentEvent OnEquipmentChanged;

	//override

	virtual void ItemRemoved(int32 EntryId) override;

	virtual void ItemAdded(int32 EntryId) override;

	virtual void ItemChanged(int32 EntryId) override;

	virtual const TArray<struct FInventoryEntry>& GetEntries() override;
};
