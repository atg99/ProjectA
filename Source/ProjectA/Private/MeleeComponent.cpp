#include "MeleeComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
// Fill out your copyright notice in the Description page of Project Settings.

// Sets default values for this component's properties
UMeleeComponent::UMeleeComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// ...
}


// Called when the game starts
void UMeleeComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UMeleeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UMeleeComponent::TryMeleeAttack()
{
	MeleeAttack();
	ServerMeleeAttack();
}

void UMeleeComponent::ServerMeleeAttack_Implementation()
{
	//MeleeAttack();
}

void UMeleeComponent::MultiMeleeAttack_Implementation()
{
}

void UMeleeComponent::MeleeAttack()
{
	if (!MeleeMontage)
	{
		return;
	}

	if (ACharacter* Character = GetCharacter())
	{
		bool bWhileAttack = Character->GetMesh()->GetAnimInstance()->Montage_IsPlaying(MeleeMontage);

		if (bWhileAttack && bIsCanCombo)
		{
			Character->GetMesh()->GetAnimInstance()->Montage_JumpToSection(ComboSections[ComboCount], MeleeMontage);
			if (ComboCount >= ComboSections.Num())
			{
				ComboCount = 0;
			}
			else
			{
				ComboCount++;
			}
		}
		else if(!bWhileAttack)
		{
			Character->PlayAnimMontage(MeleeMontage, 1, ComboSections[0]);
			ComboCount = 1;
		}
	}
}

ACharacter* UMeleeComponent::GetCharacter()
{
	return Cast<ACharacter>(GetOwner());
}
