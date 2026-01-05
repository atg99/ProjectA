// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BaseAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/ZombieEnemy.h"
#include "Utils/NetworkUtil.h"

ABaseAIController::ABaseAIController()
{
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	// 감지할 소속(Affiliation) 설정 (적, 아군, 중립 등 모두 감지하도록 설정 예시)
	//SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	//SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	//SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	/*UAISenseConfig_Sight* Sight = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight"));
	Sight->SightRadius = 300.0f;
	Sight->LoseSightRadius = 400.0f;
	Sight->PeripheralVisionAngleDegrees = 45.f;
	Sight->DetectionByAffiliation.bDetectEnemies = true;
	Sight->DetectionByAffiliation.bDetectFriendlies = false;
	Sight->DetectionByAffiliation.bDetectNeutrals = false;
	AIPerceptionComponent->ConfigureSense(*Sight);
	AIPerceptionComponent->SetDominantSense(*Sight->GetSenseImplementation());*/

	//
}

void ABaseAIController::BeginPlay()
{
	Super::BeginPlay();


}

void ABaseAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (ensure(RunBTAsset))
	{
		RunBehaviorTree(RunBTAsset);
	}

	AIPerceptionComponent->OnPerceptionUpdated.AddDynamic(this, &ABaseAIController::HandlePerceptionUpdated);
	AIPerceptionComponent->OnTargetPerceptionForgotten.AddDynamic(this, &ABaseAIController::HandleActorPerceptionForgetUpdated);
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ABaseAIController::HandleActorPerceptionUpdated);
	AIPerceptionComponent->OnTargetPerceptionInfoUpdated.AddDynamic(this, &ABaseAIController::HandleActorPerceptionInfoUpdated);

	//TeamID가 달라야지 적으로 감지 같으면 아군 없으면 중립
	SetGenericTeamId(3);

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		SetState(EMonsterState::Normal);
	}

	if (InPawn)
	{
		InPawn->OnTakeAnyDamage.AddDynamic(this, &ABaseAIController::HandleTakeAnyDamage);
	}
}

void ABaseAIController::OnUnPossess()
{
	Super::OnUnPossess();
}

void ABaseAIController::HandlePerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
}

void ABaseAIController::HandleActorPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	//GetBrainComponent();
	//GetBlackboardComponent();
	if (!Stimulus.WasSuccessfullySensed())
	{
		 if (UBlackboardComponent* BB = GetBlackboardComponent())
		 {
		 	BB->ClearValue(FName("Target"));
			SetState(EMonsterState::Normal);
		 }
		return;
	}

	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		UE_LOG(LogTemp, Log, TEXT("Sense_Sight"));
		if (UBlackboardComponent* BB = GetBlackboardComponent())
		{
			BB->SetValueAsObject(FName("Target"), Actor);
			SetState(EMonsterState::Chase);
		}
		return;
	}
}

void ABaseAIController::HandleActorPerceptionForgetUpdated(AActor* Actor)
{
}

void ABaseAIController::HandleActorPerceptionInfoUpdated(const FActorPerceptionUpdateInfo& UpdateInfo)
{
}

void ABaseAIController::SetState(EMonsterState NewEnemyState)
{
		/*	None	= 0
			Normal	= 10
			Chase	= 20
			Battle	= 30
			Death	= 40	*/
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsEnum(FName("CurrentState"), (uint8)NewEnemyState); //Chase
		MonsterState = NewEnemyState;
		if (AZombieEnemy* Zombie = Cast<AZombieEnemy>(GetPawn()))
		{
			Zombie->MonsterState = NewEnemyState;
		}
	}
}

void ABaseAIController::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	//NET_LOG(TEXT(""));
	AZombieEnemy* Zombie = Cast<AZombieEnemy>(DamagedActor);
	if (Zombie)
	{
		if (Zombie->GetCurrentHP() <= 0.f)
		{
			SetState(EMonsterState::Death);
			Zombie->StartDeath();
		}
	}
}
