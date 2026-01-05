// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SliceSystemComponent.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTA_API USliceSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USliceSystemComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 절단 시 사용할 PMC 풀 (미리 만들어두고 껐다 켰다 함)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UProceduralMeshComponent* PMC_Stump; // 몸에 붙을 쪽

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UProceduralMeshComponent* PMC_Debris; // 떨어져 나갈 쪽

	// 외부에서 함수 호출하면 알아서 처리
	UFUNCTION(BlueprintCallable, Category = "Slicing")
	void SliceBone(FName TargetBone, const FVector& HitLocation, const FVector& HitNormal, const FVector& CutNormal, float ImpulsePower);

protected:
	// 내부 로직 분리
	void SetupPMCs();

	UPROPERTY(EditAnywhere)
	UMaterialInterface* SliceCapMaterial;

		
};
