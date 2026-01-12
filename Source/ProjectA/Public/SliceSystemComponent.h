// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utils/SliceUtils.h"
#include "SliceSystemComponent.generated.h"

// 섹션별로 업데이트할 데이터를 보관하는 구조체
struct FProcMeshSectionBuffer
{
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	// UV, Color, Tangent는 변하지 않거나 빈 배열로
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	// 추가: 스키닝 계산을 위한 초기 탄젠트 저장소
	TArray<FProcMeshTangent> InitialTangents;
};

//메쉬 하나(Stump 또는 Debris)를 관리하는 통합 구조체
struct FSlicePMC
{
	//시각적 컴포넌트
	UProceduralMeshComponent* ProcMeshComp = nullptr;

	//스키닝 계산용 데이터 (Ref Pose 정보)
	TArray<FCachedSkinVertex> SkinCache;

	//GPU 전송용 버퍼 (Key: Section Index)
	TMap<int32, FProcMeshSectionBuffer> UpdateBuffers;

	bool bUpdateSkinning = false;

};

class UProceduralMeshComponent;
class UMaterialInterface;
class UDynamicMeshComponent;
class UStaticMesh;

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

	//// 절단 시 사용할 PMC 풀 (미리 만들어두고 껐다 켰다 함)
	//UPROPERTY(EditAnywhere, BlueprintReadOnly)
	//UProceduralMeshComponent* PMC_Stump; // 몸에 붙을 쪽

	//UPROPERTY(EditAnywhere, BlueprintReadOnly)
	//UProceduralMeshComponent* PMC_Debris; // 떨어져 나갈 쪽

	// 외부에서 함수 호출하면 알아서 처리
	UFUNCTION(BlueprintCallable, Category = "Slicing")
	void SliceBone(FName TargetBone, const FVector& HitLocation, const FVector& HitNormal, const FVector& CutNormal, float ImpulsePower);

	UFUNCTION(BlueprintCallable)
	void SliceBone_DMC(FName TargetBone, const FVector& HitLocation, const FVector& HitNormal, const FVector& CutNormal, float ImpulsePower);

	UFUNCTION(BlueprintCallable)
	void CopyWeightAndSlice_DMC(FName TargetBone, const FVector& HitLocation, const FVector& HitNormal, const FVector& CutNormal, float ImpulsePower);

protected:
	
	void SetupPMCs();

	void SetupDMCs();

	float GetBoneRadius(USkeletalMeshComponent* Mesh, FName BoneName);

	void RefineSkinWeights(FSlicePMC& InSlicePMC, const TSet<int32>& CutBoneIndices, bool bIsStump);

	// 초기화 함수
	void InitializePMCBuffers(FSlicePMC& InSlicePMC);

	void PrecomputeSkinningMatrices();
	// Tick에서 호출할 함수
	void UpdatePMCSkinning(FSlicePMC& InSlicePMC);

	UPROPERTY(EditAnywhere)
	UMaterialInterface* SliceCapMaterial;

	UPROPERTY(EditAnywhere)
	UStaticMesh* MeatCrossSectionMesh;

	UPROPERTY(EditAnywhere)
	TSet<FName> CutableBones;

protected:

	FSlicePMC PMC_Stump;
	FSlicePMC PMC_Debris;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UDynamicMeshComponent> DMC_Stump;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UDynamicMeshComponent> DMC_Debris;

	TArray<FMatrix> SkinningMatrices;
};
