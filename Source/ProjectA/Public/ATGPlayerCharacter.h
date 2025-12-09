// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "ATGPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UATGInventoryComponent;
struct FInputActionValue;
class AActor;
struct FInteractionData;
struct FInventoryEntry;
class UATGPlayerEquipComponent;
class UAIPerceptionStimuliSourceComponent;

UCLASS()
class PROJECTA_API AATGPlayerCharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AATGPlayerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UAIPerceptionStimuliSourceComponent* AIStimuliSourceComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UATGPlayerEquipComponent* PlayerEquipComp;

protected:

	//IA
	///** Jump Input Action */
	//UPROPERTY(EditAnywhere, Category = "Input")
	//UInputAction* JumpAction;

	///** Move Input Action */
	//UPROPERTY(EditAnywhere, Category = "Input")
	//UInputAction* MoveAction;

	///** Mouse Look Input Action */
	//UPROPERTY(EditAnywhere, Category = "Input")
	//UInputAction* MouseLookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Interaction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Inventory;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_RotateHeldItemAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_FirstMainEquip;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_SecondMainEquip;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Fire;
protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for interaction input */
	void Interact(const FInputActionValue& Value);

	/** Called for interaction input */
	void ToggleInventory(const FInputActionValue& Value);

	void RotateHeldItem(const FInputActionValue& Value);

	void TryEquipFirstMain(const FInputActionValue& Value);
	void TrySecondFirstMain(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void TryFire(const FInputActionValue& Value);

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void PutInAtInventory(FInteractionData& InteractionData);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OpenItemBox(FInteractionData& InteractionData);

	UFUNCTION(Server, Reliable)
	void ServerSetInteractActorOwner(AActor* InteractActor);

//Helper
protected:
	UATGInventoryComponent* GetInventoryComponent();

	//----------------------------------------------------------------------//
	// IGenericTeamAgentInterface 인터페이스 상속 받아야 AIPerception에서 적이나 아군으로 감지
	//----------------------------------------------------------------------//
	FGenericTeamId TeamID;
	/** Assigns Team Agent to given TeamID */
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;

	/** Retrieve team identifier in form of FGenericTeamId */
	virtual FGenericTeamId GetGenericTeamId() const override;

};
