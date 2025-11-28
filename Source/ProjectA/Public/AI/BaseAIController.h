// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ATGEnum.h"
#include "BaseAIController.generated.h"

/**
 * 
 */

class UAIPerceptionComponent;
struct FAIStimulus;
struct FActorPerceptionUpdateInfo;
class UBehaviorTree;


UCLASS()
class PROJECTA_API ABaseAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	ABaseAIController();
protected:

	virtual void BeginPlay() override;

	virtual void OnPossess(APawn* InPawn) override;

	virtual void OnUnPossess() override;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAIPerceptionComponent* AIPerceptionComponent;

	UFUNCTION()
	void HandlePerceptionUpdated(const TArray<AActor*>& UpdatedActors);
	UFUNCTION()
	void HandleActorPerceptionUpdated( AActor* Actor, FAIStimulus Stimulus);
	UFUNCTION()
	void HandleActorPerceptionForgetUpdated(AActor* Actor);

	UFUNCTION()
	void HandleActorPerceptionInfoUpdated(const FActorPerceptionUpdateInfo& UpdateInfo);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	TObjectPtr<UBehaviorTree> RunBTAsset;

	UPROPERTY(BlueprintReadOnly)
	EMonsterState MonsterState = EMonsterState::Normal;

	UFUNCTION(BlueprintCallable)
	void SetState(EMonsterState NewEnemyState);
};
