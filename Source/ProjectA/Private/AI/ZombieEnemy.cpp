// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/ZombieEnemy.h"
#include "Data/CustomDamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Utils/NetworkUtil.h"
#include "NiagaraFunctionLibrary.h"
#include "Data/ATGDamageType.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"
#include "SliceSystemComponent.h"
#include "AI/BaseAIController.h"
#include "GAS/CharacterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h" 
#include "GameplayTagContainer.h"


// Sets default values
AZombieEnemy::AZombieEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	SliceSystemComponent = CreateDefaultSubobject<USliceSystemComponent>(TEXT("SliceComponent"));

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal); // AI는 Minimal

	AttributeSet = CreateDefaultSubobject<UCharacterAttributeSet>(TEXT("AttributeSet"));
}

// Called when the game starts or when spawned
void AZombieEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AZombieEnemy::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
}

void AZombieEnemy::HandleDeath()
{
	StartDeath();
	if (ABaseAIController* AICon = Cast<ABaseAIController>(GetController()))
	{
		AICon->SetState(EMonsterState::Death);
	}
}

void AZombieEnemy::ApplyHealing(float Healing, AActor* Healer)
{
}

void AZombieEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AZombieEnemy, MonsterState);
}

// Called every frame
void AZombieEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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

//기존 TakeDamage와 GAS 호환 (임시)
float AZombieEnemy::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!AbilitySystemComponent || !DefaultDamageEffectClass)
	{
		UE_LOG(LogTemp, Error, TEXT("GAS Setup Error on %s: ASC or DefaultDamageEffectClass is MISSING!"), *GetName())
		return Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	}

	FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	ContextHandle.AddInstigator(EventInstigator ? EventInstigator->GetPawn() : DamageCauser, DamageCauser);
	
	//CDO
	UATGDamageType const* const DamageTypeCDO = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UATGDamageType>() : GetDefault<UATGDamageType>();
	//const UATGDamageType* MyDamageCDO = Cast<UATGDamageType>(DamageTypeCDO);
	
	FGameplayTagContainer OwnedTags;

	if (DamageEvent.IsOfType(FGunPointDamageEvent::ClassID))
	{
		const FGunPointDamageEvent* Event = (FGunPointDamageEvent*)(&DamageEvent);
		if (Event)
		{
			ContextHandle.AddHitResult(Event->HitInfo);
			OwnedTags = Event->OwnedTags;
			//ReceiveGunPointDamage(Event, Damage, DamageTypeCDO, Event->HitInfo.ImpactPoint, Event->HitInfo.ImpactNormal, Event->HitInfo.GetComponent(), Event->HitInfo.BoneName, Event->ShotDirection, EventInstigator, DamageCauser, Event->HitInfo);
		}
	}
	else if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		FPointDamageEvent* Event = (FPointDamageEvent*)(&DamageEvent);
		if (Event)
		{
			ContextHandle.AddHitResult(Event->HitInfo);
		}
	}
	else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
	{
		FRadialDamageEvent* Event = (FRadialDamageEvent*)(&DamageEvent);
		if (Event)
		{
			
		}
	}
	else //(DamageEvent.IsOfType(FDamageEvent::ClassID))
	{
	}

	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultDamageEffectClass, 1.0f, ContextHandle);

	if (SpecHandle.IsValid())
	{
		//데미지 수치를 SetByCaller로 전달 (GE_Damage 블루프린트에서 SetByCaller "Data.Damage"로 설정되어 있어야 함)
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.Damage.Amount")), Damage);

		SpecHandle.Data.Get()->AppendDynamicAssetTags(OwnedTags);

		//나 자신에게 적용 (이 순간 AttributeSet::PostGameplayEffectExecute가 실행됨)
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}

	return Damage;
}

void AZombieEnemy::GunPointApplyDamageMomentum(float InImpulseScale, const FVector& ShotDir, FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser, bool bScaleMomentumByMass)
{

	float const ImpulseScale = InImpulseScale;

	if ((ImpulseScale > 3.f) && (GetCharacterMovement() != nullptr))
	{
		FHitResult HitInfo;
		FVector ImpulseDir = ShotDir;
		//DamageEvent.GetBestHitInfo(this, PawnInstigator, HitInfo, ImpulseDir);

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
		//GetCharacterMovement()->Launch(Impulse);
		GetCharacterMovement()->StopMovementKeepPathing();
		GetCharacterMovement()->AddImpulse(Impulse, bMassIndependentImpulse);
		
	}
}

void AZombieEnemy::ReceiveGunPointDamage(const struct FGunPointDamageEvent* Event, float Damage, const UATGDamageType* DamageType, FVector HitLocation, FVector HitNormal, UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, AController* InstigatedBy, AActor* DamageCauser, const FHitResult& HitInfo)
{
	NET_LOG(TEXT("FGunPointDamageEvent"));
	if (!Event)
	{
		return;
	}

	MultiPlayEffectHitReact(DamageType, HitLocation, HitNormal, Event->HitInfo.BoneName, Event->ImpulseScale);
	GunPointApplyDamageMomentum(Event->ImpulseScale, Event->ShotDirection, *Event, InstigatedBy->GetPawn(), DamageCauser, true);
	CurrentHP -= Damage;

}

void AZombieEnemy::MultiPlayEffectHitReact_Implementation(const class UATGDamageType* DamageType, const FVector& HitLocation, const FVector& HitNormal, const FName& BoneName, float ImpulseScale)
{
	//!HasAuthority() || GetNetMode() == NM_ListenServer
	if (!IsRunningDedicatedServer())
	{
		UNiagaraSystem* SelectedVFX = nullptr;
		switch (DamageType->DamageType)
		{
		case EDamageType::Normal:
		{
			SelectedVFX = NormalDamageImpactVFX;
			NET_LOG(TEXT("Normal Damage VFX"));
			break;
		}
		case EDamageType::Fire:
		{
			SelectedVFX = FireDamageImpactVFX;
			NET_LOG(TEXT("Fire Damage VFX"));
			break;
		}
		default:
		{
			SelectedVFX = NormalDamageImpactVFX;
			NET_LOG(TEXT("Default Damage VFX"));
			break;
		}
		}

		FRotator HitRotator = UKismetMathLibrary::MakeRotFromX(HitNormal);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), SelectedVFX, HitLocation, HitRotator);

	
	}

	LastDamageCapture.BoneName = BoneName;
	LastDamageCapture.HitLocation = HitLocation;
	LastDamageCapture.HitNormal = HitNormal;
	LastDamageCapture.ImpulseScale = ImpulseScale;

	//GetMesh()->GlobalAnimRateScale = 0;
	CustomTimeDilation = 0.01f;
	FTimerHandle HitStopHandle;
	GetWorld()->GetTimerManager().SetTimer(
		HitStopHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				//GetMesh()->GlobalAnimRateScale = 1.f;
				this->CustomTimeDilation = 1.f;
			}),
		0.03f,
		false
	);

}

void AZombieEnemy::StartSlice()
{
	//bp에서 호출 죽음이벤트
	//절단
	if (SliceSystemComponent)
	{
		SliceSystemComponent->SliceBone(LastDamageCapture.BoneName, LastDamageCapture.HitLocation, LastDamageCapture.HitNormal, LastDamageCapture.ImpulseScale);
	}
}

void AZombieEnemy::MultiStartDeath_Implementation()
{

}
