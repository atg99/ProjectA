// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGPlayerCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"

#include "ATGInterface.h"
#include "ATGInventoryComponent.h"
#include "ATGContainerComponent.h"
#include "GameFramework/PlayerState.h"

#include "ATGEnum.h"
#include "Data/ATGItemData.h"
#include "Data/ATGWeaponData.h"
#include "Data/ATGConsumableItemData.h"
#include "ATGPlayerController.h"
#include "ATGHUDComponent.h"
#include "GameFramework/HUD.h"
#include "ATGPlayerEquipComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Utils/NetworkUtil.h"
#include "MeleeComponent.h"

// Sets default values
AATGPlayerCharacter::AATGPlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	AIStimuliSourceComp = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("AIPerceptionStimuliSourceComponent"));

	PlayerEquipComp = CreateDefaultSubobject<UATGPlayerEquipComponent>(TEXT("PlayerEquipComp"));

	MeleeComp = CreateDefaultSubobject<UMeleeComponent>(TEXT("MeleeComponent"));

	SetGenericTeamId(1);
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

}

// Called when the game starts or when spawned
void AATGPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AATGPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AATGPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		//// Jumping
		//EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		//EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//// Moving
		//EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AATGPlayerCharacter::Move);
		//
		//// Looking
		//EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AATGPlayerCharacter::Look);

		//Interaction
		EnhancedInputComponent->BindAction(IA_Interaction, ETriggerEvent::Started, this, &AATGPlayerCharacter::Interact);

		//Inventory
		EnhancedInputComponent->BindAction(IA_Inventory, ETriggerEvent::Started, this, &AATGPlayerCharacter::ToggleInventory);

		//RotateItem
		EnhancedInputComponent->BindAction(IA_RotateHeldItemAction, ETriggerEvent::Started, this, &AATGPlayerCharacter::RotateHeldItem);

		EnhancedInputComponent->BindAction(IA_FirstMainEquip, ETriggerEvent::Started, this, &AATGPlayerCharacter::TryEquipFirstMain);

		EnhancedInputComponent->BindAction(IA_SecondMainEquip, ETriggerEvent::Started, this, &AATGPlayerCharacter::TrySecondFirstMain);

		EnhancedInputComponent->BindAction(IA_Unarmed, ETriggerEvent::Started, this, &AATGPlayerCharacter::TryMelee);

		EnhancedInputComponent->BindAction(IA_MeleeAttack, ETriggerEvent::Started, this, &AATGPlayerCharacter::TryMeleeAttack);

		EnhancedInputComponent->BindAction(IA_Fire, ETriggerEvent::Started, this, &AATGPlayerCharacter::TryFire);
		EnhancedInputComponent->BindAction(IA_Fire, ETriggerEvent::Completed, this, &AATGPlayerCharacter::StopFire);

		EnhancedInputComponent->BindAction(IA_GunAim, ETriggerEvent::Started, this, &AATGPlayerCharacter::TryGunAim);
		EnhancedInputComponent->BindAction(IA_GunAim, ETriggerEvent::Completed, this, &AATGPlayerCharacter::StopGunAim);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AATGPlayerCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AATGPlayerCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}



void AATGPlayerCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AATGPlayerCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AATGPlayerCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AATGPlayerCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

//Fire Logic
void AATGPlayerCharacter::TryFire(const FInputActionValue& Value)
{
	if (PlayerEquipComp)
	{
		//Bullet Manager 에서 Parallel Simulation
		PlayerEquipComp->TryFire();

		//블루프린트에서 애니메이션 제어용 
		OnCharacterFire();
	}
}

void AATGPlayerCharacter::StopFire(const FInputActionValue& Value)
{
	
}

void AATGPlayerCharacter::RecoverMoveAnim()
{
	OnCharacterRecoverFromFire();
}

void AATGPlayerCharacter::TryGunAim(const FInputActionValue& Value)
{
	if (PlayerEquipComp)
	{
		PlayerEquipComp->ReadyToFire();
	}
}

void AATGPlayerCharacter::StopGunAim(const FInputActionValue& Value)
{
	if (PlayerEquipComp)
	{
		PlayerEquipComp->ReleaseAim();
	}
}

void AATGPlayerCharacter::Interact(const FInputActionValue& Value)
{
	TArray<FOverlapResult> Overlaps;

	const FVector Center = GetMesh()->GetComponentLocation();

	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);  // 등등

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OverlapSphere), false);
	QueryParams.AddIgnoredActor(GetOwner());

	bool bAny = GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		Center,
		FQuat::Identity,
		ObjParams,
		FCollisionShape::MakeSphere(100),
		QueryParams
	);

	DrawDebugSphere(GetWorld(), Center, 100.f, 16, FColor::Cyan, false, 2.f);

	if (!bAny)
	{
		return;
	}

	float MinDist = 9999999.f;
	UActorComponent* NearestInterfaceActorComp = nullptr;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		TSet<UActorComponent*> ActorComps = Overlap.GetActor()->GetComponents();
		
		for (auto Comp : ActorComps)
		{
			if (Comp->GetClass()->ImplementsInterface(UATGInterface::StaticClass()))
			{
				/*if (GEngine)
					GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, Comp->GetReadableName());*/

				float Dist = FVector::DistSquared(Center, Overlap.GetActor()->GetActorLocation());
				if (Dist < MinDist)
				{
					MinDist = Dist;
					NearestInterfaceActorComp = Comp;
				}
			}
		}
	}

	if (NearestInterfaceActorComp)
	{
		auto ATGInterface = Cast<IATGInterface>(NearestInterfaceActorComp);

		FInteractionData InteractionData;

		InteractionData.InteractingActor = this;

		ATGInterface->PlayerInteract(InteractionData);

		switch (InteractionData.InteractionType)
		{
		case EInteractionType::PickUpItem:
			PutInAtInventory(InteractionData);
			break;
		case  EInteractionType::ItemGridBox:
			OpenItemBox(InteractionData);
			break;
		case EInteractionType::Equipment:
			break;
		default:
			break;
		}		
	}
	else
	{
		/*if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("There is no NearestInterfaceActorComp"));*/
	}
}

void AATGPlayerCharacter::ToggleInventory(const FInputActionValue& Value)
{
	//UE_LOG(LogTemp, Display, TEXT("ToggleInventory"));
	if (UATGHUDComponent* HUDComp = Cast<AATGPlayerController>(GetController())->GetHUD()->FindComponentByClass<UATGHUDComponent>())
	{
		HUDComp->ToggleInventoryUI();
	}
	else
	{
		//UE_LOG(LogTemp, Error, TEXT("Can't Find HUDComponent at HUD"));
	}
}

void AATGPlayerCharacter::RotateHeldItem(const FInputActionValue& Value)
{
	//UATGInventoryComponent* InventoryComp = GetInventoryComponent();
	//if (InventoryComp)
	//{
	//	InventoryComp->OnItemRotated.Broadcast(-1);
	//}
	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("RotateItem"));
}

void AATGPlayerCharacter::TryEquipFirstMain(const FInputActionValue& Value)
{
	//UE_LOG(LogTemp, Warning, TEXT("!!! AATGPlayerCharacter::TryEquipFirstMain"));
	if (PlayerEquipComp)
	{
		//UE_LOG(LogTemp, Warning, TEXT("!!! PlayerEquipComp Valid"));
		PlayerEquipComp->TryChangePlayerUsingSlot(EEquipmentSlotType::MainWeapon1);
	}
}

void AATGPlayerCharacter::TrySecondFirstMain(const FInputActionValue& Value)
{
	if (PlayerEquipComp)
	{
		PlayerEquipComp->TryChangePlayerUsingSlot(EEquipmentSlotType::MainWeapon2);
	}
}

void AATGPlayerCharacter::TryMelee(const FInputActionValue& Value)
{
	if (PlayerEquipComp)
	{
		PlayerEquipComp->TryChangePlayerUsingSlot(EEquipmentSlotType::MeleeWeapon);
	}
}

void AATGPlayerCharacter::TryMeleeAttack(const FInputActionValue& Value)
{
	NET_LOG("");
	if (MeleeComp)
	{
		MeleeComp->MeleeAttack();
	}
	//PlayAnimMontage();
}

void AATGPlayerCharacter::PutInAtInventory(FInteractionData& InteractionData)
{
	UATGInventoryComponent* InventoryComp = GetInventoryComponent();
	if (InventoryComp)
	{
	/*	if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("InventoryComp Found"));*/
		//load asset  InterationData.ItemDef.LoadSynchronous()
		
		InventoryComp->TryPickupClient(InteractionData.ItemDef, InteractionData.ItemQty, InteractionData.InteractedActor);
	}
}

void AATGPlayerCharacter::OpenItemBox(FInteractionData& InteractionData)
{
	if (UATGContainerComponent* ContainerComp = Cast<UATGContainerComponent>(InteractionData.InteractedComponent))
	{
		if (UATGHUDComponent* HUDComp = Cast<AATGPlayerController>(GetController())->GetHUD()->FindComponentByClass<UATGHUDComponent>())
		{
			ServerSetInteractActorOwner(InteractionData.InteractedActor);
			HUDComp->OnContainerToggle.Broadcast(ContainerComp);
		}
	}
}

void AATGPlayerCharacter::ServerSetInteractActorOwner_Implementation(AActor* InteractActor)
{
	InteractActor->SetOwner(this);
}

UATGInventoryComponent* AATGPlayerCharacter::GetInventoryComponent()
{
	UActorComponent* Comp = GetPlayerState()->GetComponentByClass(UATGInventoryComponent::StaticClass());
	UATGInventoryComponent* InventoryComp = Cast<UATGInventoryComponent>(Comp);
	return InventoryComp;
}

void AATGPlayerCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	TeamID = NewTeamID;
}

FGenericTeamId AATGPlayerCharacter::GetGenericTeamId() const
{
	return TeamID;
}


