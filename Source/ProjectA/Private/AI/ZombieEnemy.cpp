// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/ZombieEnemy.h"
#include "Data/CustomDamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NetworkUtil.h"

// Sets default values
AZombieEnemy::AZombieEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AZombieEnemy::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AZombieEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AZombieEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

float AZombieEnemy::TryPlayMontage(UAnimMontage* Montage, float PlayRate, FName StartSessionName)
{
	float Duration = PlayAnimMontage(Montage, PlayRate, StartSessionName);
	//서버에서 검사
	if (Duration > 0.f)
	{
		MultiPlayMontage(Montage, PlayRate, StartSessionName);
	}
	return Duration;
}

void AZombieEnemy::MultiPlayMontage_Implementation(UAnimMontage* Montage, float PlayRate, FName StartSessionName)
{
	//중복실행방지
	if (HasAuthority())
	{
		return;
	}
	PlayAnimMontage(Montage, PlayRate, StartSessionName);
	return;
}

void AZombieEnemy::TryStopMontage(UAnimMontage* Montage)
{
	MultiStopMontage(Montage);
}

void AZombieEnemy::MultiStopMontage_Implementation(UAnimMontage* Montage)
{
	StopAnimMontage(Montage);
	return;
}

float AZombieEnemy::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	if (CurrentHP <= 0)
	{
		return Damage;
	}
	if (DamageEvent.IsOfType(FGunPointDamageEvent::ClassID))
	{
		FGunPointDamageEvent* Event = (FGunPointDamageEvent*)(&DamageEvent);
		if (Event)
		{
			NET_LOG(TEXT("FGunPointDamageEvent"));
			CApplyDamageMomentum(Event->ImpulseScale, DamageEvent, EventInstigator->GetPawn(), DamageCauser, true);
		}

	}
	else if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		FPointDamageEvent* Event = (FPointDamageEvent*)(&DamageEvent);
		if (Event)
		{
			CurrentHP -= Damage;
		}
		//SpawnHitEffect(Event->HitInfo);
		//UE_LOG(LogTemp, Warning, TEXT("Point Damage %f %s"), Damage, *Event->DamageTypeClass->GetName());
	}
	else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
	{
		FRadialDamageEvent* Event = (FRadialDamageEvent*)(&DamageEvent);
		if (Event)
		{
			CurrentHP -= Damage;
		}
	}
	else //(DamageEvent.IsOfType(FDamageEvent::ClassID))
	{
		CurrentHP -= Damage;
	}

	return Damage;
}

void AZombieEnemy::CApplyDamageMomentum(float InImpulseScale, FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser, bool bScaleMomentumByMass)
{
	
	float const ImpulseScale = InImpulseScale;

	if ((ImpulseScale > 3.f) && (GetCharacterMovement() != nullptr))
	{
		FHitResult HitInfo;
		FVector ImpulseDir;
		DamageEvent.GetBestHitInfo(this, PawnInstigator, HitInfo, ImpulseDir);

		FVector Impulse = ImpulseDir * ImpulseScale;
		bool const bMassIndependentImpulse = bScaleMomentumByMass;

		// limit Z momentum added if already going up faster than jump (to avoid blowing character way up into the sky)
		{
			FVector MassScaledImpulse = Impulse;
			if (!bMassIndependentImpulse && GetCharacterMovement()->Mass > UE_SMALL_NUMBER)
			{
				MassScaledImpulse = MassScaledImpulse / GetCharacterMovement()->Mass;
			}

			if ((GetCharacterMovement()->Velocity.Z > GetDefault<UCharacterMovementComponent>(GetCharacterMovement()->GetClass())->JumpZVelocity) && (MassScaledImpulse.Z > 0.f))
			{
				Impulse.Z *= 0.5f;
			}
		}

		GetCharacterMovement()->AddImpulse(Impulse, bMassIndependentImpulse);
	}
}