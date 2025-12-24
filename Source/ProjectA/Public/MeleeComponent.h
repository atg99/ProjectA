// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MeleeComponent.generated.h"

class ACharacter;
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTA_API UMeleeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UMeleeComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim")
	UAnimMontage* MeleeMontage;
public:
	bool bIsCanCombo = false;
	int ComboCount = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim")
	TArray<FName> ComboSections;

	UFUNCTION()
	void TryMeleeAttack();

	UFUNCTION(Server, Reliable)
	void ServerMeleeAttack();

	UFUNCTION(NetMulticast, Reliable)
	void MultiMeleeAttack();

	void MeleeAttack();

	ACharacter* GetCharacter();


		
};
