// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProceduralMeshComponent.h"
#include "SliceUtils.generated.h"

/**
 * 
 */

 // 엣지를 정의하는 구조체 (해시맵 키로 사용)
struct FMeshEdge
{
    int32 VertA;
    int32 VertB;

    FMeshEdge(int32 A, int32 B)
    {
        // 순서가 달라도 같은 엣지로 취급하기 위해 정렬
        VertA = FMath::Min(A, B);
        VertB = FMath::Max(A, B);
    }

    bool operator==(const FMeshEdge& Other) const
    {
        return VertA == Other.VertA && VertB == Other.VertB;
    }

    friend uint32 GetTypeHash(const FMeshEdge& Edge)
    {
        return HashCombine(GetTypeHash(Edge.VertA), GetTypeHash(Edge.VertB));
    }
};

//weight cache
struct FCachedSkinVertex
{
    FVector InitialPos;      // 초기 위치 (Ref Pose)
    FVector InitialNormal;   // 초기 노멀
    int32 SectionIndex;      // PMC의 몇 번째 섹션인지
    int32 VertIndex;         // 해당 섹션의 몇 번째 버텍스인지
    int32 BoneIndices[4];    // 본 인덱스
    float BoneWeights[4];    // 본 웨이트
};

class UProceduralMeshComponent;
class UDynamicMeshComponent;

UCLASS()
class PROJECTA_API USliceUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Slicing")
	static void ConvertBoneToProcMesh(USkeletalMeshComponent* SkeletalMeshComp, FName BoneName, UProceduralMeshComponent* ProcMeshComp);

	UFUNCTION(BlueprintCallable, Category = "Slicing")
	static void ConvertBoneToProcMesh_2(USkeletalMeshComponent* SkeletalMeshComp, FName BoneName, UProceduralMeshComponent* ProcMeshComp);

	UFUNCTION(BlueprintCallable, Category = "Slicing")
	static void MaskTargetBoneOnly(USkeletalMeshComponent* SkeletalMeshComp, FName TargetBoneName);
	
    UFUNCTION(BlueprintCallable, Category = "Slicing")
    static void ConvertDynamicMeshToProcMesh(UDynamicMeshComponent* DynamicMeshComp, UProceduralMeshComponent* ProcMeshComp);

    static void ConvertDynamicMeshToProcMesh(UDynamicMeshComponent* DynamicMeshComp, UProceduralMeshComponent* ProcMeshComp, TArray<FCachedSkinVertex>& OutCache);

    UFUNCTION(BlueprintCallable, Category = "Slicing")
    static void InitializeDMCFromSkeletalMesh(UDynamicMeshComponent* DMC, USkeletalMeshComponent* SkelMeshComp, EGeometryScriptOutcomePins& OutCome);

    UFUNCTION(BlueprintCallable, Category = "Slicing")
    static void ApplySkinningWithDMCData(UDynamicMeshComponent* DMC, USkeletalMeshComponent* SkelMeshComp);
    
private:
    // 구멍을 찾아서 메우는 함수
    static void CloseMeshHoles(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FProcMeshTangent>& Tangents, TArray<FLinearColor>& Colors);
};
