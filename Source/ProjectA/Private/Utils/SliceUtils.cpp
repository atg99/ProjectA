// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/SliceUtils.h"
#include "ProceduralMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshRenderData.h" 
#include "Rendering/PositionVertexBuffer.h"
#include "Rendering/StaticMeshVertexBuffer.h" 
#include "KismetProceduralMeshLibrary.h"
#include "GeometryScript/MeshQueryFunctions.h"
#include "GeometryScript/MeshAssetFunctions.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Components/DynamicMeshComponent.h"
#include "GeometryScript/GeometryScriptTypes.h"

#include "Rendering/SkeletalMeshLODRenderData.h"
#include "SkeletalMeshLODRenderDataToDynamicMesh.h" 

#include "MeshDescription.h" 
#include "SkeletalMeshAttributes.h"

#include "Kismet/KismetMathLibrary.h" 
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"

#include "DynamicMesh/MeshAttributeUtil.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/DynamicVertexSkinWeightsAttribute.h"
#include "Operations/TransferBoneWeights.h"

//#include "AnimationCore/Public/BoneWeights.h"


void USliceUtils::ConvertBoneToProcMesh(USkeletalMeshComponent* SkeletalMeshComp, FName BoneName, UProceduralMeshComponent* ProcMeshComp)
{
    if (!SkeletalMeshComp || !ProcMeshComp) return;

    USkeletalMesh* SkelMesh = SkeletalMeshComp->GetSkeletalMeshAsset();
    if (!SkelMesh) return;

    // 1. 타겟 본의 실제 인덱스 (RefSkeleton 기준)
    int32 TargetBoneIndex = SkeletalMeshComp->GetBoneIndex(BoneName);
    if (TargetBoneIndex == INDEX_NONE) return;

    // 2. 렌더 데이터 접근 (LOD 0)
    FSkeletalMeshRenderData* RenderData = SkelMesh->GetResourceForRendering();
    if (!RenderData || !RenderData->LODRenderData.IsValidIndex(0)) return;

    const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[0];
    const FSkinWeightVertexBuffer& SkinWeightBuffer = LODData.SkinWeightVertexBuffer;
    const FPositionVertexBuffer& PosBuffer = LODData.StaticVertexBuffers.PositionVertexBuffer;
    const FStaticMeshVertexBuffer& StaticMeshBuffer = LODData.StaticVertexBuffers.StaticMeshVertexBuffer;

    // 타겟 본의 Reference Pose(Bind Pose) Component Transform 계산
    // 버텍스 버퍼는 Bind Pose 기준이므로, Bind Pose 상태의 본 위치를 알아내야 함
    FTransform BoneToComponent = FTransform::Identity;
    const FReferenceSkeleton& RefSkeleton = SkelMesh->GetRefSkeleton();

    int32 CurrentBoneIndex = TargetBoneIndex;
    while (CurrentBoneIndex != INDEX_NONE)
    {
        const FTransform& LocalTransform = RefSkeleton.GetRefBonePose()[CurrentBoneIndex];
        BoneToComponent = BoneToComponent * LocalTransform; // Local -> Parent -> ... -> Component
        CurrentBoneIndex = RefSkeleton.GetParentIndex(CurrentBoneIndex);
    }

    // Component Space -> Bone Local Space 변환 행렬
    FTransform WorldToLocal = BoneToComponent.Inverse();

    // 기존 PMC 초기화 (필요시)
    ProcMeshComp->ClearAllMeshSections();

    // 3. 섹션(머터리얼) 단위 순회
    for (int32 SectionIdx = 0; SectionIdx < LODData.RenderSections.Num(); SectionIdx++)
    {
        const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIdx];

        TArray<FVector> NewVertices;
        TArray<int32> NewTriangles;
        TArray<FVector> NewNormals;
        TArray<FVector2D> NewUVs;
        TArray<FProcMeshTangent> NewTangents;
        TArray<FLinearColor> NewColors; // 버텍스 컬러

        // 중복 버텍스 생성을 방지하기 위한 맵 (OriginalIndex -> NewIndex)
        TMap<int32, int32> VertexMap;

        // [핵심 수정] 위치가 겹치는 버텍스를 찾기 위한 맵
        // Key: 버텍스 위치 (FVector), Value: 새로 생성된 NewVertices의 인덱스
        //TMap<FVector, int32> UniquePosMap;

        // 4. 삼각형 단위 순회
        for (uint32 TriIdx = 0; TriIdx < Section.NumTriangles; TriIdx++)
        {
            // 인덱스 버퍼에서 삼각형 구성하는 버텍스 인덱스 가져오기
            uint32 BaseIndex = Section.BaseIndex + TriIdx * 3;
            int32 Index0 = LODData.MultiSizeIndexContainer.GetIndexBuffer()->Get(BaseIndex + 0);
            int32 Index1 = LODData.MultiSizeIndexContainer.GetIndexBuffer()->Get(BaseIndex + 1);
            int32 Index2 = LODData.MultiSizeIndexContainer.GetIndexBuffer()->Get(BaseIndex + 2);

            int32 Indices[3] = { Index0, Index1, Index2 };
            bool bIsTargetBone = false;

            // 5. 웨이트 체크 (BoneMap 변환 로직 추가됨)
            for (int32 i = 0; i < 3; i++)
            {
                int32 OriginalVertIdx = Indices[i];
                int32 NumInfluences = SkinWeightBuffer.GetMaxBoneInfluences();

                for (int32 InfluenceIdx = 0; InfluenceIdx < NumInfluences; InfluenceIdx++)
                {
                    // [중요 수정] 버퍼의 본 인덱스를 가져옴
                    int32 BufferBoneIndex = SkinWeightBuffer.GetBoneIndex(OriginalVertIdx, InfluenceIdx);
                    float Weight = SkinWeightBuffer.GetBoneWeight(OriginalVertIdx, InfluenceIdx);

                    // 웨이트가 거의 없으면 무시
                    if (Weight < 0.01f) continue;

                    // [핵심 수정] Section에 BoneMap이 있다면 변환해야 실제 BoneIndex가 됨
                    int32 RealBoneIndex = BufferBoneIndex;
                    if (Section.BoneMap.Num() > 0 && Section.BoneMap.IsValidIndex(BufferBoneIndex))
                    {
                        RealBoneIndex = Section.BoneMap[BufferBoneIndex];
                    }

                    // 타겟 본과 일치하는지 확인
                    if (RealBoneIndex == TargetBoneIndex)
                    {
                        bIsTargetBone = true;
                        break;
                    }
                }
                if (bIsTargetBone) break;
            }

            // 6. 타겟 본에 속한 삼각형이라면 데이터 복사
            if (bIsTargetBone)
            {
                //for (int32 i = 0; i < 3; i++)
                //{
                //    int32 OriginalIdx = Indices[i];

                //    // 위치 좌표 변환
                //    FVector Pos = (FVector)PosBuffer.VertexPosition(OriginalIdx);
                //    Pos = WorldToLocal.TransformPosition(Pos);

                //    // [핵심 해결책]
                //    // 기존: 인덱스(OriginalIdx)로 중복 검사 -> UV 경계선에서 버텍스가 쪼개짐 -> Cap 실패
                //    // 변경: 위치(Pos)로 중복 검사 -> UV가 달라도 위치가 같으면 하나로 합침 -> Cap 성공

                //    if (UniquePosMap.Contains(Pos))
                //    {
                //        // 이미 같은 위치에 점이 있다면 그 인덱스를 재사용 (용접)
                //        NewTriangles.Add(UniquePosMap[Pos]);
                //    }
                //    else
                //    {
                //        // 새 점 생성
                //        FVector Normal = (FVector)StaticMeshBuffer.VertexTangentZ(OriginalIdx);
                //        Normal = WorldToLocal.TransformVector(Normal);

                //        FVector TangentVec = (FVector)StaticMeshBuffer.VertexTangentX(OriginalIdx);
                //        TangentVec = WorldToLocal.TransformVector(TangentVec);
                //        FProcMeshTangent Tangent(TangentVec, false);

                //        FVector2D UV = (FVector2D)StaticMeshBuffer.GetVertexUV(OriginalIdx, 0);
                //        FLinearColor VertColor = FLinearColor::White;

                //        int32 NewIdx = NewVertices.Add(Pos);
                //        NewNormals.Add(Normal);
                //        NewUVs.Add(UV);
                //        NewTangents.Add(Tangent);
                //        NewColors.Add(VertColor);

                //        // 맵에 등록 (위치 -> 새 인덱스)
                //        UniquePosMap.Add(Pos, NewIdx);
                //        NewTriangles.Add(NewIdx);
                //    }
                //}
          

                for (int32 i = 0; i < 3; i++)
                {
                    int32 OriginalIdx = Indices[i];

                    // 이미 추가된 버텍스라면 인덱스만 재사용
                    if (VertexMap.Contains(OriginalIdx))
                    {
                        NewTriangles.Add(VertexMap[OriginalIdx]);
                    }
                    else
                    {
                        //좌표 및 벡터 변환 적용
                        // 위치: Component Space -> Bone Local Space
                        FVector Pos = (FVector)PosBuffer.VertexPosition(OriginalIdx);
                        Pos = WorldToLocal.TransformPosition(Pos);

                        // 노멀: 회전 변환 적용
                        FVector Normal = (FVector)StaticMeshBuffer.VertexTangentZ(OriginalIdx);
                        Normal = WorldToLocal.TransformVector(Normal);

                        // 탄젠트
                        FVector TangentVec = (FVector)StaticMeshBuffer.VertexTangentX(OriginalIdx);
                        TangentVec = WorldToLocal.TransformVector(TangentVec);
                        FProcMeshTangent Tangent(TangentVec, false);

                        FVector2D UV = (FVector2D)StaticMeshBuffer.GetVertexUV(OriginalIdx, 0);

                        // 버텍스 컬러: 기본 흰색 (보이게 설정)
                        FLinearColor VertColor = FLinearColor::White;

                        int32 NewIdx = NewVertices.Add(Pos);
                        NewNormals.Add(Normal);
                        NewUVs.Add(UV);
                        NewTangents.Add(Tangent);
                        NewColors.Add(VertColor);

                        VertexMap.Add(OriginalIdx, NewIdx);
                        NewTriangles.Add(NewIdx);
                    }
                }
            }
        }

        // 7. PMC 섹션 생성 및 머터리얼 할당
        if (NewVertices.Num() > 0)
        {
            ProcMeshComp->CreateMeshSection_LinearColor(
                SectionIdx,
                NewVertices,
                NewTriangles,
                NewNormals,
                NewUVs,
                TArray<FVector2D>(), // UV1
                TArray<FVector2D>(), // UV2
                TArray<FVector2D>(), // UV3
                NewColors,// Colors
                NewTangents,
                true // Collision
            );

            // 머터리얼 할당 로직
            // Section.MaterialIndex는 스켈레탈 메시의 Materials 배열 인덱스입니다.
            if (SkelMesh->GetMaterials().IsValidIndex(Section.MaterialIndex))
            {
                UMaterialInterface* TargetMat = SkeletalMeshComp->GetMaterial(Section.MaterialIndex);
                if (TargetMat)
                {
                    ProcMeshComp->SetMaterial(SectionIdx, TargetMat);
                }
            }
        }
    }
}

void USliceUtils::ConvertBoneToProcMesh_2(USkeletalMeshComponent* SkeletalMeshComp, FName BoneName, UProceduralMeshComponent* ProcMeshComp)
{
    if (!SkeletalMeshComp || !ProcMeshComp) return;

    USkeletalMesh* SkelMesh = SkeletalMeshComp->GetSkeletalMeshAsset();
    if (!SkelMesh) return;

    int32 TargetBoneIndex = SkeletalMeshComp->GetBoneIndex(BoneName);
    if (TargetBoneIndex == INDEX_NONE) return;

    FSkeletalMeshRenderData* RenderData = SkelMesh->GetResourceForRendering();
    if (!RenderData || !RenderData->LODRenderData.IsValidIndex(0)) return;

    const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[0];
    const FSkinWeightVertexBuffer& SkinWeightBuffer = LODData.SkinWeightVertexBuffer;
    const FPositionVertexBuffer& PosBuffer = LODData.StaticVertexBuffers.PositionVertexBuffer;
    const FStaticMeshVertexBuffer& StaticMeshBuffer = LODData.StaticVertexBuffers.StaticMeshVertexBuffer;

    // Bone Transform 계산
    FTransform BoneToComponent = FTransform::Identity;
    const FReferenceSkeleton& RefSkeleton = SkelMesh->GetRefSkeleton();
    int32 CurrentBoneIndex = TargetBoneIndex;
    while (CurrentBoneIndex != INDEX_NONE)
    {
        const FTransform& LocalTransform = RefSkeleton.GetRefBonePose()[CurrentBoneIndex];
        BoneToComponent = BoneToComponent * LocalTransform;
        CurrentBoneIndex = RefSkeleton.GetParentIndex(CurrentBoneIndex);
    }
    FTransform WorldToLocal = BoneToComponent.Inverse();

    ProcMeshComp->ClearAllMeshSections();

    for (int32 SectionIdx = 0; SectionIdx < LODData.RenderSections.Num(); SectionIdx++)
    {
        const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIdx];

        TArray<FVector> NewVertices;
        TArray<int32> NewTriangles;
        TArray<FVector> NewNormals;
        TArray<FVector2D> NewUVs;
        TArray<FProcMeshTangent> NewTangents;
        TArray<FLinearColor> NewColors;

        // [핵심 수정] 오차 허용 용접을 위한 맵
        // Key: FIntVector (좌표에 1000을 곱해 정수화, 0.001 단위 오차 무시)
        TMap<FIntVector, int32> WeldedVertexMap;

        for (uint32 TriIdx = 0; TriIdx < Section.NumTriangles; TriIdx++)
        {
            uint32 BaseIndex = Section.BaseIndex + TriIdx * 3;
            int32 Indices[3];
            Indices[0] = LODData.MultiSizeIndexContainer.GetIndexBuffer()->Get(BaseIndex + 0);
            Indices[1] = LODData.MultiSizeIndexContainer.GetIndexBuffer()->Get(BaseIndex + 1);
            Indices[2] = LODData.MultiSizeIndexContainer.GetIndexBuffer()->Get(BaseIndex + 2);

            bool bIsTargetBone = false;

            // 웨이트 체크
            for (int32 i = 0; i < 3; i++)
            {
                int32 OriginalVertIdx = Indices[i];
                int32 NumInfluences = SkinWeightBuffer.GetMaxBoneInfluences();

                for (int32 InfluenceIdx = 0; InfluenceIdx < NumInfluences; InfluenceIdx++)
                {
                    int32 BufferBoneIndex = SkinWeightBuffer.GetBoneIndex(OriginalVertIdx, InfluenceIdx);
                    float Weight = SkinWeightBuffer.GetBoneWeight(OriginalVertIdx, InfluenceIdx);

                    if (Weight < 0.01f) continue;

                    int32 RealBoneIndex = BufferBoneIndex;
                    if (Section.BoneMap.Num() > 0 && Section.BoneMap.IsValidIndex(BufferBoneIndex))
                    {
                        RealBoneIndex = Section.BoneMap[BufferBoneIndex];
                    }

                    if (RealBoneIndex == TargetBoneIndex)
                    {
                        bIsTargetBone = true;
                        break;
                    }
                }
                if (bIsTargetBone) break;
            }

            if (bIsTargetBone)
            {
                for (int32 i = 0; i < 3; i++)
                {
                    int32 OriginalIdx = Indices[i];

                    FVector Pos = (FVector)PosBuffer.VertexPosition(OriginalIdx);
                    Pos = WorldToLocal.TransformPosition(Pos);

                    // [핵심 해결] 위치를 정수화하여 미세 오차 무시 (소수점 3자리 정도까지 정밀도 유지)
                    // 100.0f ~ 1000.0f 정도를 곱해서 비교
                    FIntVector PosKey = FIntVector(FMath::RoundToInt(Pos.X * 1000.f), FMath::RoundToInt(Pos.Y * 1000.f), FMath::RoundToInt(Pos.Z * 1000.f));

                    if (WeldedVertexMap.Contains(PosKey))
                    {
                        // 이미 존재하면 인덱스 재사용 (완벽한 용접)
                        NewTriangles.Add(WeldedVertexMap[PosKey]);
                    }
                    else
                    {
                        // 없다면 새로 추가
                        // *주의: 노멀은 나중에 재계산하므로 여기서는 더미나 원본을 넣음
                        FVector Normal = FVector::UpVector;
                        FProcMeshTangent Tangent(FVector::RightVector, false);
                        FVector2D UV = (FVector2D)StaticMeshBuffer.GetVertexUV(OriginalIdx, 0);
                        FLinearColor VertColor = FLinearColor::White;

                        int32 NewIdx = NewVertices.Add(Pos);
                        NewNormals.Add(Normal);
                        NewUVs.Add(UV);
                        NewTangents.Add(Tangent);
                        NewColors.Add(VertColor);

                        WeldedVertexMap.Add(PosKey, NewIdx);
                        NewTriangles.Add(NewIdx);
                    }
                }
            }
        }

        // 데이터가 있을 때만 섹션 생성
        if (NewVertices.Num() > 0 && NewTriangles.Num() > 0)
        {
            CloseMeshHoles(NewVertices, NewTriangles, NewNormals, NewUVs, NewTangents, NewColors);

            // [중요] 용접으로 인해 노멀이 깨질 수 있으므로 강제 재계산
            // 이 함수는 용접된 메쉬의 표면을 따라 부드러운 노멀을 새로 만듭니다.
            UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
                NewVertices,
                NewTriangles,
                NewUVs,
                NewNormals,
                NewTangents
            );

            ProcMeshComp->CreateMeshSection_LinearColor(
                SectionIdx,
                NewVertices,
                NewTriangles,
                NewNormals,
                NewUVs,
                TArray<FVector2D>(),
                TArray<FVector2D>(),
                TArray<FVector2D>(),
                NewColors,
                NewTangents,
                true
            );

            // 머터리얼 할당
            if (SkelMesh->GetMaterials().IsValidIndex(Section.MaterialIndex))
            {
                ProcMeshComp->SetMaterial(SectionIdx, SkelMesh->GetMaterials()[Section.MaterialIndex].MaterialInterface);
            }
        }
    }
}

void USliceUtils::MaskTargetBoneOnly(USkeletalMeshComponent* SkeletalMeshComp, FName TargetBoneName)
{
    if (!SkeletalMeshComp) return;

    // (필수) 독립적인 처리를 위해 에셋이 복제되었는지 확인 권장
    USkeletalMesh* SkelMesh = SkeletalMeshComp->GetSkeletalMeshAsset();
    if (!SkelMesh) return;

    // 1. 렌더 데이터 접근
    FSkeletalMeshRenderData* RenderData = SkelMesh->GetResourceForRendering();
    if (!RenderData || !RenderData->LODRenderData.IsValidIndex(0)) return;

    FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[0];
    const FSkinWeightVertexBuffer& SkinWeightBuffer = LODData.SkinWeightVertexBuffer;

    // 2. 타겟 본 인덱스 찾기
    int32 TargetBoneIndex = SkeletalMeshComp->GetBoneIndex(TargetBoneName);
    if (TargetBoneIndex == INDEX_NONE) return;

    // 3. 컬러 버퍼 초기화 (없으면 흰색으로 생성)
    FColorVertexBuffer& ColorBuffer = LODData.StaticVertexBuffers.ColorVertexBuffer;
    uint32 NumVertices = LODData.StaticVertexBuffers.PositionVertexBuffer.GetNumVertices();

    if (ColorBuffer.GetNumVertices() != NumVertices)
    {
        ColorBuffer.Init(NumVertices);
        for (uint32 i = 0; i < NumVertices; i++)
        {
            ColorBuffer.VertexColor(i) = FColor::White;
        }
    }

    bool bModified = false;

    // 4. 섹션 순회
    for (const FSkelMeshRenderSection& Section : LODData.RenderSections)
    {
        for (uint32 i = 0; i < Section.NumVertices; i++)
        {
            int32 VertexIndex = Section.BaseVertexIndex + i;

            // 이미 투명하면 패스
            if (ColorBuffer.VertexColor(VertexIndex).A == 0) continue;

            // ---------------------------------------------------------
            // 타겟 본 웨이트가 0.01 이상이면 무조건 숨김
            // ---------------------------------------------------------
            bool bShouldMask = false;
            int32 NumInfluences = SkinWeightBuffer.GetMaxBoneInfluences();

            for (int32 InfIdx = 0; InfIdx < NumInfluences; InfIdx++)
            {
                // 웨이트 가져오기 (엔진 버전에 따라 반환 타입이 다를 수 있으나, 기존 코드의 float 형식을 따름)
                float Weight = SkinWeightBuffer.GetBoneWeight(VertexIndex, InfIdx);

                // 웨이트가 0.5보다 작으면 무시 (영향력이 거의 없는 경우)
                if (Weight < 0.5f)
                {
                    continue;
                }

                int32 BoneIndexInBuffer = SkinWeightBuffer.GetBoneIndex(VertexIndex, InfIdx);

                // BoneMap 변환 (Section의 로컬 인덱스 -> 실제 스켈레톤 인덱스)
                int32 RealBoneIndex = BoneIndexInBuffer;
                if (Section.BoneMap.Num() > 0 && Section.BoneMap.IsValidIndex(BoneIndexInBuffer))
                {
                    RealBoneIndex = Section.BoneMap[BoneIndexInBuffer];
                }

                // 현재 영향력을 주는 본이 타겟 본과 일치하는가?
                if (RealBoneIndex == TargetBoneIndex)
                {
                    bShouldMask = true;
                    break; // 하나라도 조건에 맞으면 더 이상 검사할 필요 없음
                }
            }

            // 타겟 본의 영향력이 조금이라도 있으면 투명화
            if (bShouldMask)
            {
                FColor NewColor = ColorBuffer.VertexColor(VertexIndex);
                NewColor.A = 0; // 투명화
                ColorBuffer.VertexColor(VertexIndex) = NewColor;
                bModified = true;
            }
        }
    }

    // 5. GPU 업데이트
    if (bModified)
    {
        BeginInitResource(&ColorBuffer);
        ENQUEUE_RENDER_COMMAND(UpdateSkelMeshColorBuffer)(
            [&LODData](FRHICommandListImmediate& RHICmdList)
            {
                LODData.StaticVertexBuffers.ColorVertexBuffer.UpdateRHI(RHICmdList);
            });
        SkeletalMeshComp->MarkRenderStateDirty();
    }
}

void USliceUtils::MaskTargetBoneOnly_2(USkeletalMeshComponent* SkeletalMeshComp, FName TargetBoneName)
{
    if (!SkeletalMeshComp) return;

    USkeletalMesh* SkelMesh = SkeletalMeshComp->GetSkeletalMeshAsset();
    if (!SkelMesh) return;

    FSkeletalMeshRenderData* RenderData = SkelMesh->GetResourceForRendering();
    if (!RenderData || !RenderData->LODRenderData.IsValidIndex(0)) return;

    FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[0];
    const FSkinWeightVertexBuffer& SkinWeightBuffer = LODData.SkinWeightVertexBuffer;

    int32 TargetBoneIndex = SkeletalMeshComp->GetBoneIndex(TargetBoneName);
    if (TargetBoneIndex == INDEX_NONE) return;

    uint32 NumVertices = LODData.GetNumVertices();

    // 1. 컴포넌트의 LODInfo가 생성되어 있는지 확인 (보통 BeginPlay 이후엔 존재함)
    if (!SkeletalMeshComp->LODInfo.IsValidIndex(0))
    {
        return;
    }
    FSkelMeshComponentLODInfo& LODInfo = SkeletalMeshComp->LODInfo[0];

    // 2. 이 컴포넌트(인스턴스) 전용 버텍스 컬러 오버라이드 버퍼를 준비합니다.
    bool bIsFirstTime = false;
    if (!LODInfo.OverrideVertexColors)
    {
        LODInfo.OverrideVertexColors = new FColorVertexBuffer();
        bIsFirstTime = true;
    }

    // 버텍스 컬러를 쓸 임시 배열 (이미 마스킹된 부분 누적을 위해)
    TArray<FColor> NewColors;
    NewColors.Init(FColor::White, NumVertices);

    if (!bIsFirstTime && LODInfo.OverrideVertexColors->GetNumVertices() == NumVertices)
    {
        for (uint32 i = 0; i < NumVertices; i++)
        {
            NewColors[i] = LODInfo.OverrideVertexColors->VertexColor(i);
        }
    }

    bool bModified = false;

    // 3. 기존에 작성하셨던 완벽한 스킨 웨이트 검사 로직
    for (const FSkelMeshRenderSection& Section : LODData.RenderSections)
    {
        for (uint32 i = 0; i < Section.NumVertices; i++)
        {
            int32 VertexIndex = Section.BaseVertexIndex + i;

            if (NewColors[VertexIndex].A == 0) continue; // 이미 숨겨진 곳은 패스

            bool bShouldMask = false;
            int32 NumInfluences = SkinWeightBuffer.GetMaxBoneInfluences();

            for (int32 InfIdx = 0; InfIdx < NumInfluences; InfIdx++)
            {
                float Weight = SkinWeightBuffer.GetBoneWeight(VertexIndex, InfIdx);
                if (Weight < 0.5f) continue; // 작성하신 대로 0.5 이상만 날림

                int32 BoneIndexInBuffer = SkinWeightBuffer.GetBoneIndex(VertexIndex, InfIdx);
                int32 RealBoneIndex = BoneIndexInBuffer;

                if (Section.BoneMap.Num() > 0 && Section.BoneMap.IsValidIndex(BoneIndexInBuffer))
                {
                    RealBoneIndex = Section.BoneMap[BoneIndexInBuffer];
                }

                if (RealBoneIndex == TargetBoneIndex)
                {
                    bShouldMask = true;
                    break;
                }
            }

            if (bShouldMask)
            {
                NewColors[VertexIndex].A = 0; // 투명화 (알파 0)
                bModified = true;
            }
        }
    }

    // 4. 컴포넌트 전용 버퍼 업데이트 (렌더 스레드 안전성 보장)
    if (bModified)
    {
        LODInfo.OverrideVertexColors->InitFromColorArray(NewColors);

        if (bIsFirstTime)
        {
            // 처음 생성되었을 때 리소스 초기화
            BeginInitResource(LODInfo.OverrideVertexColors);
        }
        else
        {
            // 이미 존재하는 리소스라면 데이터만 업데이트
            BeginUpdateResourceRHI(LODInfo.OverrideVertexColors);
        }

        // 변경 사항을 GPU에 강제로 다시 그리라고 지시
        SkeletalMeshComp->MarkRenderStateDirty();
    }
}

void USliceUtils::ConvertDynamicMeshToProcMesh(UDynamicMeshComponent* DynamicMeshComp, UProceduralMeshComponent* ProcMeshComp)
{
    if (!DynamicMeshComp || !ProcMeshComp) return;

    // 1. 초기화
    ProcMeshComp->ClearAllMeshSections();

    // 2. DynamicMesh 데이터 접근
    UDynamicMesh* DynMesh = DynamicMeshComp->GetDynamicMesh();
    if (!DynMesh) return;

    // ProcessMesh를 통해 로우 레벨 메시 데이터(FDynamicMesh3)에 접근
    DynMesh->ProcessMesh([&](const UE::Geometry::FDynamicMesh3& Mesh)
        {
            // Weight
            struct FProcMeshVertexWeight
            {
                int32 BoneIndex[4]; // 최대 4개의 뼈
                float BoneWeight[4];
            };

            // 머터리얼 ID 별로 데이터를 모으기 위한 구조체
            struct FMeshSectionData
            {
                TArray<FVector> Vertices;
                TArray<int32> Triangles;
                TArray<FVector> Normals;
                TArray<FVector2D> UVs;
                TArray<FProcMeshTangent> Tangents;
                TArray<FLinearColor> Colors;
                TMap<int32, int32> VertexMap; // OldIndex -> NewIndex
                TArray<FProcMeshVertexWeight> SkinWeights;
            };

            TMap<int32, FMeshSectionData> Sections;

            // 3. 머터리얼 속성 확인 (없으면 기본값 0 사용)
            bool bHasMaterials = Mesh.HasAttributes() && Mesh.Attributes()->HasMaterialID();
            const auto* MaterialIDAttrib = bHasMaterials ? Mesh.Attributes()->GetMaterialID() : nullptr;

            // 4. 노멀/UV/Tangent 속성 접근
            bool bHasNormals = Mesh.HasVertexNormals();
            bool bHasUVs = Mesh.HasAttributes() && Mesh.Attributes()->NumUVLayers() > 0;
            const auto* UVAttrib = bHasUVs ? Mesh.Attributes()->GetUVLayer(0) : nullptr;
            // GeometryScript는 보통 오버레이 노멀을 사용함
            const auto* NormalAttrib = Mesh.HasAttributes() ? Mesh.Attributes()->PrimaryNormals() : nullptr;

            // 5. 삼각형 순회
            for (int32 TriID : Mesh.TriangleIndicesItr())
            {
                // 이 삼각형의 머터리얼 ID 가져오기
                int32 MatID = 0;
                if (MaterialIDAttrib)
                {
                    MatID = MaterialIDAttrib->GetValue(TriID);
                }

                FMeshSectionData& Section = Sections.FindOrAdd(MatID);
                UE::Geometry::FIndex3i TriVerts = Mesh.GetTriangle(TriID);

                // 삼각형 구성 (0, 1, 2)
                for (int32 j = 0; j < 3; j++)
                {
                    int32 VertID = TriVerts[j];

                    // PMC는 버텍스 공유를 인덱스로 처리하므로, 같은 위치/속성의 버텍스는 재활용해야 함.
                    // 하지만 여기서는 단순화를 위해 삼각형마다(Flat) 혹은 GeometryScript의 Topology 그대로 복사.
                    // DynamicMesh는 'Compact' 하지 않을 수 있으므로 인덱스 매핑이 필요함.

                    // 간단한 구현: 이미 처리된 VertID라면 인덱스만 추가 (Topology 유지)
                    if (Section.VertexMap.Contains(VertID))
                    {
                        Section.Triangles.Add(Section.VertexMap[VertID]);
                    }
                    else
                    {
                        // 위치
                        FVector Pos = (FVector)Mesh.GetVertex(VertID);

                        // 노멀
                        FVector Normal = FVector::UpVector;
                        if (NormalAttrib)
                        {
                            // 오버레이 노멀은 요소 ID 기반
                            int32 ElemID = NormalAttrib->GetElementIDAtVertex(TriID, j); // TriID와 코너 인덱스로 찾음
                            if (ElemID != -1/*UE::Geometry::FIndexConstants::InvalidID*/)
                            {
                                Normal = (FVector)NormalAttrib->GetElement(ElemID);
                            }
                        }
                        else if (bHasNormals)
                        {
                            Normal = (FVector)Mesh.GetVertexNormal(VertID);
                        }

                        // UV
                        FVector2D UV = FVector2D::ZeroVector;
                        if (UVAttrib)
                        {
                            int32 ElemID = UVAttrib->GetElementIDAtVertex(TriID, j);
                            if (ElemID != -1)
                            {
                                UV = (FVector2D)UVAttrib->GetElement(ElemID);
                            }
                        }

                        // 탄젠트 (없으면 자동 계산 혹은 임시값)
                        FProcMeshTangent Tangent(FVector::ForwardVector, false);
                        // 필요 시 GeometryScriptLibrary_MeshTangentsFunctions::ComputeTangents 사용 후 가져와야 함

                        int32 NewIndex = Section.Vertices.Add(Pos);
                        Section.Normals.Add(Normal);
                        Section.UVs.Add(UV);
                        Section.Tangents.Add(Tangent);
                        Section.Colors.Add(FLinearColor::White); // 버텍스 컬러 기본값

                        Section.VertexMap.Add(VertID, NewIndex);
                        Section.Triangles.Add(NewIndex);
                    }
                }
            }

            // 6. PMC 섹션 생성
            for (auto& Elem : Sections)
            {
                int32 MatIndex = Elem.Key;
                FMeshSectionData& Data = Elem.Value;

                if (Data.Vertices.Num() > 0)
                {
                    ProcMeshComp->CreateMeshSection_LinearColor(
                        MatIndex,
                        Data.Vertices,
                        Data.Triangles,
                        Data.Normals,
                        Data.UVs,
                        TArray<FVector2D>(), TArray<FVector2D>(), TArray<FVector2D>(),
                        Data.Colors,
                        Data.Tangents,
                        true // Collision
                    );

                    // 머터리얼 할당
                    if (DynamicMeshComp->GetMaterial(MatIndex))
                    {
                        ProcMeshComp->SetMaterial(MatIndex, DynamicMeshComp->GetMaterial(MatIndex));
                    }
                }
            }
        });
}

void USliceUtils::ConvertDynamicMeshToProcMesh(UDynamicMeshComponent* DynamicMeshComp, UProceduralMeshComponent* ProcMeshComp, TArray<FCachedSkinVertex>& OutCache)
{
    if (!DynamicMeshComp || !ProcMeshComp) return;

    ProcMeshComp->ClearAllMeshSections();
    OutCache.Reset(); // 캐시 초기화

    UDynamicMesh* DynMesh = DynamicMeshComp->GetDynamicMesh();
    if (!DynMesh) return;

    DynMesh->ProcessMesh([&](const UE::Geometry::FDynamicMesh3& Mesh)
        {
            // 1. 스킨 웨이트 속성 가져오기 (가장 중요!)
            const auto* SkinWeightsAttr = Mesh.Attributes()->GetSkinWeightsAttribute(FSkeletalMeshAttributes::DefaultSkinWeightProfileName);

            // 웨이트가 없다면
            if (!SkinWeightsAttr)
            {
                UE_LOG(LogTemp, Warning, TEXT("Warning: No SkinWeights attribute found on DynamicMesh!"));
            }

            struct FMeshSectionData
            {
                TArray<FVector> Vertices;
                TArray<int32> Triangles;
                TArray<FVector> Normals;
                TArray<FVector2D> UVs;
                TArray<FProcMeshTangent> Tangents;
                TArray<FLinearColor> Colors;
                TMap<int32, int32> VertexMap; // Old(DynamicMesh) -> New(PMC) Index
            };

            TMap<int32, FMeshSectionData> Sections;

            // 머터리얼, 노멀, UV 속성 접근
            bool bHasMaterials = Mesh.HasAttributes() && Mesh.Attributes()->HasMaterialID();
            const auto* MaterialIDAttrib = bHasMaterials ? Mesh.Attributes()->GetMaterialID() : nullptr;

            bool bHasNormals = Mesh.HasVertexNormals();
            const auto* NormalAttrib = Mesh.HasAttributes() ? Mesh.Attributes()->PrimaryNormals() : nullptr;

            bool bHasUVs = Mesh.HasAttributes() && Mesh.Attributes()->NumUVLayers() > 0;
            const auto* UVAttrib = bHasUVs ? Mesh.Attributes()->GetUVLayer(0) : nullptr;

            // 삼각형 순회
            for (int32 TriID : Mesh.TriangleIndicesItr())
            {
                int32 MatID = 0;
                if (MaterialIDAttrib)
                {
                    MatID = MaterialIDAttrib->GetValue(TriID);
                }

                FMeshSectionData& Section = Sections.FindOrAdd(MatID);
                UE::Geometry::FIndex3i TriVerts = Mesh.GetTriangle(TriID);

                for (int32 j = 0; j < 3; j++)
                {
                    int32 VertID = TriVerts[j];

                    // 이미 처리된 버텍스라면 인덱스만 재사용
                    if (Section.VertexMap.Contains(VertID))
                    {
                        Section.Triangles.Add(Section.VertexMap[VertID]);
                    }
                    else
                    {
                        // --- 1. 기본 정보 추출 ---
                        FVector Pos = (FVector)Mesh.GetVertex(VertID);

                        FVector Normal = FVector::UpVector;
                        if (NormalAttrib)
                        {
                            int32 ElemID = NormalAttrib->GetElementIDAtVertex(TriID, j);
                            if (ElemID != -1) Normal = (FVector)NormalAttrib->GetElement(ElemID);
                        }
                        else if (bHasNormals)
                        {
                            Normal = (FVector)Mesh.GetVertexNormal(VertID);
                        }

                        FVector2D UV = FVector2D::ZeroVector;
                        if (UVAttrib)
                        {
                            int32 ElemID = UVAttrib->GetElementIDAtVertex(TriID, j);
                            if (ElemID != -1) UV = (FVector2D)UVAttrib->GetElement(ElemID);
                        }

                        FProcMeshTangent Tangent(FVector::ForwardVector, false);

                        // --- 2. PMC 데이터 채우기 ---
                        int32 NewIndex = Section.Vertices.Add(Pos);
                        Section.Normals.Add(Normal);
                        Section.UVs.Add(UV);
                        Section.Tangents.Add(Tangent);
                        Section.Colors.Add(FLinearColor::White);
                        Section.VertexMap.Add(VertID, NewIndex);
                        Section.Triangles.Add(NewIndex);

                        // --- 3. [핵심] 스킨 웨이트 캐싱 (OutCache에 저장) ---
                        FCachedSkinVertex CachedVert;
                        CachedVert.InitialPos = Pos;       // T-Pose 위치 저장
                        CachedVert.InitialNormal = Normal; // T-Pose 노멀 저장
                        CachedVert.SectionIndex = MatID;   // 나중에 어떤 섹션을 업데이트할지 알기 위해
                        CachedVert.VertIndex = NewIndex;   // 해당 섹션의 몇 번째 버텍스인지

                        // 초기화
                        for (int k = 0; k < 4; k++) { CachedVert.BoneIndices[k] = 0; CachedVert.BoneWeights[k] = 0.f; }

                        if (SkinWeightsAttr)
                        {

                            //UE::Geometry::FSkinWeights Weights;
                            UE::AnimationCore::FBoneWeights Weights;
                            SkinWeightsAttr->GetValue(VertID, Weights);

                            int32 NumWeights = FMath::Min(Weights.Num(), 4);
                            for (int32 k = 0; k < NumWeights; ++k)
                            {
                                //TArray<FBoneWeight>를 감싸고 있는 컨테이너
                                //FBoneWeight 객체를 가져옴
                                const UE::AnimationCore::FBoneWeight& BoneWeightObj = Weights[k];

                                // 2. 객체 내부의 Getter 함수를 사용해 값을 추출합니다.
                                CachedVert.BoneIndices[k] = BoneWeightObj.GetBoneIndex();
                                CachedVert.BoneWeights[k] = BoneWeightObj.GetWeight();
                            }
                        }

                        // 외부 배열에 추가
                        OutCache.Add(CachedVert);
                    }
                }
            }

            // PMC 섹션 생성
            for (auto& Elem : Sections)
            {
                int32 MatIndex = Elem.Key;
                FMeshSectionData& Data = Elem.Value;

                if (Data.Vertices.Num() > 0)
                {
                    ProcMeshComp->CreateMeshSection_LinearColor(
                        MatIndex,
                        Data.Vertices,
                        Data.Triangles,
                        Data.Normals,
                        Data.UVs,
                        TArray<FVector2D>(), TArray<FVector2D>(), TArray<FVector2D>(),
                        Data.Colors,
                        Data.Tangents,
                        true
                    );

                    if (DynamicMeshComp->GetMaterial(MatIndex))
                    {
                        ProcMeshComp->SetMaterial(MatIndex, DynamicMeshComp->GetMaterial(MatIndex));
                    }
                }
            }
        });
}

void USliceUtils::ConvertDynamicMeshToProcMesh(UDynamicMeshComponent* DynamicMeshComp, UProceduralMeshComponent* ProcMeshComp, TArray<FCachedSkinVertex>& OutCache, const TSet<int32>& AllowedBoneIndices)
{
    // 1. 기본 유효성 검사
    if (!DynamicMeshComp || !ProcMeshComp) return;

    ProcMeshComp->ClearAllMeshSections();
    OutCache.Reset();

    UDynamicMesh* DynMesh = DynamicMeshComp->GetDynamicMesh();
    if (!DynMesh) return;

    // 2. Dynamic Mesh 처리
    DynMesh->ProcessMesh([&](const UE::Geometry::FDynamicMesh3& Mesh)
        {
            // 속성 세트 확인
            const UE::Geometry::FDynamicMeshAttributeSet* Attributes = Mesh.Attributes();
            bool bHasAttributes = Mesh.HasAttributes();

            // [수정 1] Overlay 포인터를 미리 가져옵니다. (FDynamicMesh3는 Overlay 방식으로 UV/Normal을 관리함)
            const UE::Geometry::FDynamicMeshUVOverlay* UVOverlay = bHasAttributes ? Attributes->PrimaryUV() : nullptr;
            const UE::Geometry::FDynamicMeshNormalOverlay* NormalOverlay = bHasAttributes ? Attributes->PrimaryNormals() : nullptr;
            const UE::Geometry::FDynamicMeshColorOverlay* ColorOverlay = bHasAttributes ? Attributes->PrimaryColors() : nullptr;
            const UE::Geometry::FDynamicMeshMaterialAttribute* MaterialIDAttrib = (bHasAttributes && Attributes->HasMaterialID()) ? Attributes->GetMaterialID() : nullptr;

            // 스킨 웨이트
            const auto* SkinWeightsAttr = bHasAttributes ? Attributes->GetSkinWeightsAttribute(FSkeletalMeshAttributes::DefaultSkinWeightProfileName) : nullptr;

            // PMC 데이터 구조체
            struct FMeshSectionData
            {
                TArray<FVector> Vertices;
                TArray<int32> Triangles;
                TArray<FVector> Normals;
                TArray<FVector2D> UVs;
                TArray<FProcMeshTangent> Tangents;
                TArray<FLinearColor> Colors;
                TMap<int32, TArray<int32>> VertexMap; // VertID -> SectionVertIndices
            };
            TMap<int32, FMeshSectionData> Sections;

            // 3. 삼각형 순회
            for (int32 TriID : Mesh.TriangleIndicesItr())
            {
                UE::Geometry::FIndex3i TriVerts = Mesh.GetTriangle(TriID);

                // [수정 2] Overlay에서 현재 삼각형의 Element ID들을 가져옵니다.
                // Overlay는 토폴로지가 다를 수 있으므로(UV Island 등) 삼각형 ID로 접근해야 합니다.
                UE::Geometry::FIndex3i UVTri = (UVOverlay && UVOverlay->IsSetTriangle(TriID)) ? UVOverlay->GetTriangle(TriID) : UE::Geometry::FIndex3i::Invalid();
                UE::Geometry::FIndex3i NormalTri = (NormalOverlay && NormalOverlay->IsSetTriangle(TriID)) ? NormalOverlay->GetTriangle(TriID) : UE::Geometry::FIndex3i::Invalid();
                UE::Geometry::FIndex3i ColorTri = (ColorOverlay && ColorOverlay->IsSetTriangle(TriID)) ? ColorOverlay->GetTriangle(TriID) : UE::Geometry::FIndex3i::Invalid();

                // --- 필터링 로직 (Bone Index 기준) ---
                bool bKeepTriangle = false;
                int32 MatID = (MaterialIDAttrib) ? MaterialIDAttrib->GetValue(TriID) : 0;

                if (SkinWeightsAttr)
                {
                    for (int32 j = 0; j < 3; j++)
                    {
                        int32 VertID = TriVerts[j];
                        UE::AnimationCore::FBoneWeights Weights;
                        SkinWeightsAttr->GetValue(VertID, Weights);

                        float TotalWeight = 0.f;
                        bool bHasAllowedBone = false;

                        for (const auto& BW : Weights)
                        {
                            if (BW.GetWeight() > 0.001f)
                            {
                                TotalWeight += BW.GetWeight();
                                if (AllowedBoneIndices.Contains(BW.GetBoneIndex()))
                                {
                                    bHasAllowedBone = true;
                                }
                            }
                        }

                        // [조건 A] 절단면(Cap) - 웨이트가 거의 없으면 유지 (Sliced Geometry)
                        if (TotalWeight < 0.001f)
                        {
                            bKeepTriangle = true;
                            break;
                        }
                        // [조건 B] 허용된 본 포함
                        if (bHasAllowedBone)
                        {
                            bKeepTriangle = true;
                            break;
                        }
                    }
                }
                else
                {
                    bKeepTriangle = true; // StaticMesh라면 무조건 유지
                }

                if (!bKeepTriangle) continue;

                // --- 섹션 데이터 채우기 ---
                FMeshSectionData& Section = Sections.FindOrAdd(MatID);

                for (int32 j = 0; j < 3; j++)
                {
                    int32 VertID = TriVerts[j];

                    // (A) 위치
                    FVector Pos = (FVector)Mesh.GetVertex(VertID);

                    // (B) [수정 3] Overlay를 통한 UV/Normal/Color 추출 로직 개선

                    // UV
                    FVector2D UV = FVector2D::ZeroVector;
                    if (UVTri[j] != UE::Geometry::FDynamicMesh3::InvalidID)
                    {
                        UV = (FVector2D)UVOverlay->GetElement(UVTri[j]);
                    }

                    // Normal
                    FVector Normal = FVector::UpVector;
                    if (NormalTri[j] != UE::Geometry::FDynamicMesh3::InvalidID)
                    {
                        Normal = (FVector)NormalOverlay->GetElement(NormalTri[j]);
                    }
                    else
                    {
                        // Overlay 노멀이 없으면 버텍스 노멀 사용
                        if (Mesh.HasVertexNormals())
                        {
                            Normal = (FVector)Mesh.GetVertexNormal(VertID);
                        }
                    }
                    // 노멀 안전장치 (0 벡터 방지)
                    if (Normal.IsNearlyZero()) Normal = FVector::UpVector;
                    else Normal.Normalize();

                    // Color
                    FLinearColor VertColor = FLinearColor::White; // 기본값 White로 설정하여 곱해질 때 검은색 방지
                    if (ColorTri[j] != UE::Geometry::FDynamicMesh3::InvalidID)
                    {
                        // DynamicMesh는 보통 Vector4f로 저장됨
                        UE::Math::TVector4<float> ColVec = ColorOverlay->GetElement(ColorTri[j]);
                        VertColor = FLinearColor(ColVec.X, ColVec.Y, ColVec.Z, ColVec.W);
                    }


                    // (C) 버텍스 중복 검사 (Vertex Split 처리)
                    int32 FinalVertIndex = -1;
                    if (Section.VertexMap.Contains(VertID))
                    {
                        const TArray<int32>& Candidates = Section.VertexMap[VertID];
                        for (int32 ExistingIdx : Candidates)
                        {
                            // 위치는 같지만 UV/Normal이 다른 경우(Hard Edge, UV Seam) 분리해야 함
                            if (Section.UVs[ExistingIdx].Equals(UV, 1e-4f) &&
                                Section.Normals[ExistingIdx].Equals(Normal, 1e-4f) &&
                                Section.Colors[ExistingIdx].Equals(VertColor, 1e-4f))
                            {
                                FinalVertIndex = ExistingIdx;
                                break;
                            }
                        }
                    }

                    // (D) 새 버텍스 추가
                    if (FinalVertIndex == -1)
                    {
                        FinalVertIndex = Section.Vertices.Add(Pos);
                        Section.Normals.Add(Normal);
                        Section.UVs.Add(UV);
                        Section.Colors.Add(VertColor);
                        Section.Tangents.Add(FProcMeshTangent(0, 0, 1)); // 계산 전 더미값

                        Section.VertexMap.FindOrAdd(VertID).Add(FinalVertIndex);

                        // 스킨 웨이트 캐싱
                        FCachedSkinVertex CachedVert;
                        CachedVert.InitialPos = Pos;
                        CachedVert.InitialNormal = Normal;
                        CachedVert.SectionIndex = MatID;
                        CachedVert.VertIndex = FinalVertIndex;
                        for (int k = 0; k < 4; k++) { CachedVert.BoneIndices[k] = 0; CachedVert.BoneWeights[k] = 0.f; }

                        if (SkinWeightsAttr)
                        {
                            UE::AnimationCore::FBoneWeights Weights;
                            SkinWeightsAttr->GetValue(VertID, Weights);
                            int32 WriteIdx = 0;
                            for (int32 k = 0; k < Weights.Num() && WriteIdx < 4; ++k)
                            {
                                const auto& BW = Weights[k];
                                if (BW.GetWeight() > 0.001f)
                                {
                                    CachedVert.BoneIndices[WriteIdx] = BW.GetBoneIndex();
                                    CachedVert.BoneWeights[WriteIdx] = BW.GetWeight();
                                    WriteIdx++;
                                }
                            }
                        }
                        OutCache.Add(CachedVert);
                    }

                    Section.Triangles.Add(FinalVertIndex);
                }
            }

            // 4. Procedural Mesh Section 생성
            for (auto& Elem : Sections)
            {
                int32 MatIndex = Elem.Key;
                FMeshSectionData& Data = Elem.Value;

                if (Data.Vertices.Num() > 0)
                {
                    // (A) 탄젠트 계산 (이제 UV가 정상적이므로 제대로 계산됨)
                    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
                        Data.Vertices,
                        Data.Triangles,
                        Data.UVs,
                        Data.Normals,
                        Data.Tangents
                    );

                    // (B) 생성
                    ProcMeshComp->CreateMeshSection_LinearColor(
                        MatIndex,
                        Data.Vertices,
                        Data.Triangles,
                        Data.Normals,
                        Data.UVs,
                        {}, {}, {},
                        Data.Colors,
                        Data.Tangents,
                        true
                    );

                    // (C) 머터리얼 할당
                    UMaterialInterface* TargetMat = nullptr;
                    if (MatIndex >= 0 && MatIndex < DynamicMeshComp->GetNumMaterials())
                    {
                        TargetMat = DynamicMeshComp->GetMaterial(MatIndex);
                    }
                    if (!TargetMat && DynamicMeshComp->GetNumMaterials() > 0)
                    {
                        TargetMat = DynamicMeshComp->GetMaterial(0);
                    }
                    if (TargetMat)
                    {
                        ProcMeshComp->SetMaterial(MatIndex, TargetMat);
                    }
                }
            }
        });
}

//void USliceUtils::ConvertDynamicMeshToProcMesh(UDynamicMeshComponent* DynamicMeshComp, UProceduralMeshComponent* ProcMeshComp, TArray<FCachedSkinVertex>& OutCache, const TSet<int32>& AllowedBoneIndices)
//{
//    // 1. 기본 유효성 검사
//    if (!DynamicMeshComp || !ProcMeshComp) return;
//
//    ProcMeshComp->ClearAllMeshSections();
//    OutCache.Reset();
//
//    UDynamicMesh* DynMesh = DynamicMeshComp->GetDynamicMesh();
//    if (!DynMesh) return;
//
//    // 2. Dynamic Mesh 처리 (Lambda)
//    DynMesh->ProcessMesh([&](const UE::Geometry::FDynamicMesh3& Mesh)
//        {
//            // 스킨 웨이트 속성
//            const auto* SkinWeightsAttr = Mesh.Attributes()->GetSkinWeightsAttribute(FSkeletalMeshAttributes::DefaultSkinWeightProfileName);
//            if (!Mesh.HasAttributes())
//            {
//                //Mesh.EnableAttributes(); // 속성이 없다면 켭니다 (보통 이미 있음)
//            }
//            const UE::Geometry::FDynamicMeshAttributeSet* Attributes = Mesh.Attributes();
//            const UE::Geometry::FDynamicMeshUVOverlay* UVOverlay = Attributes->PrimaryUV();
//
//            // PMC 생성을 위한 임시 데이터 구조체
//            struct FMeshSectionData
//            {
//                TArray<FVector> Vertices;
//                TArray<int32> Triangles;
//                TArray<FVector> Normals;
//                TArray<FVector2D> UVs;
//                TArray<FProcMeshTangent> Tangents;
//                TArray<FLinearColor> Colors;
//
//                // [중요] VertID(위치인덱스) -> PMC 버텍스 인덱스들 (속성이 다르면 여러개일 수 있음)
//                TMap<int32, TArray<int32>> VertexMap;
//            };
//            TMap<int32, FMeshSectionData> Sections;
//
//            // 속성 접근자 준비
//            bool bHasMaterials = Mesh.HasAttributes() && Mesh.Attributes()->HasMaterialID();
//            const auto* MaterialIDAttrib = bHasMaterials ? Mesh.Attributes()->GetMaterialID() : nullptr;
//
//            const auto* NormalAttrib = Mesh.HasAttributes() ? Mesh.Attributes()->PrimaryNormals() : nullptr;
//            bool bHasNormals = Mesh.HasVertexNormals();
//
//            bool bHasUVs = Mesh.HasAttributes() && Mesh.Attributes()->NumUVLayers() > 0;
//            const auto* UVAttrib = bHasUVs ? Mesh.Attributes()->GetUVLayer(0) : nullptr;
//
//            bool bHasColors = Mesh.HasAttributes() && Mesh.Attributes()->HasPrimaryColors();
//            const auto* ColorAttrib = bHasColors ? Mesh.Attributes()->PrimaryColors() : nullptr;
//
//            // ---------------------------------------------------------------------
//            // 3. 삼각형 순회 및 데이터 추출
//            // ---------------------------------------------------------------------
//            for (int32 TriID : Mesh.TriangleIndicesItr())
//            {
//                UE::Geometry::FIndex3i TriVerts = Mesh.GetTriangle(TriID);
//
//                // --- 필터링 로직 (Bone Index 기준) ---
//                bool bKeepTriangle = false;
//
//                // 머터리얼 ID (없으면 0번)
//                int32 MatID = 0;
//                if (MaterialIDAttrib) MatID = MaterialIDAttrib->GetValue(TriID);
//
//                if (SkinWeightsAttr)
//                {
//                    for (int32 j = 0; j < 3; j++)
//                    {
//                        int32 VertID = TriVerts[j];
//                        UE::AnimationCore::FBoneWeights Weights;
//                        SkinWeightsAttr->GetValue(VertID, Weights);
//
//                        float TotalWeight = 0.f;
//                        bool bHasAllowedBone = false;
//
//                        for (const auto& BW : Weights)
//                        {
//                            float W = BW.GetWeight();
//                            if (W > 0.001f)
//                            {
//                                TotalWeight += W;
//                                if (AllowedBoneIndices.Contains(BW.GetBoneIndex()))
//                                {
//                                    bHasAllowedBone = true;
//                                }
//                            }
//                        }
//
//                        // [조건 A] 절단면(Cap) - 웨이트가 거의 없음
//                        if (TotalWeight < 0.001f)
//                        {
//                            bKeepTriangle = true;
//                            break;
//                        }
//                        // [조건 B] 허용된 본 포함
//                        if (bHasAllowedBone)
//                        {
//                            bKeepTriangle = true;
//                            break;
//                        }
//                    }
//                }
//                else
//                {
//                    // 웨이트가 없으면(Static Mesh 등) 모두 렌더링
//                    bKeepTriangle = true;
//                }
//
//                if (!bKeepTriangle) continue;
//
//
//                // --- 섹션 데이터 채우기 ---
//                FMeshSectionData& Section = Sections.FindOrAdd(MatID);
//
//                for (int32 j = 0; j < 3; j++)
//                {
//                    int32 VertID = TriVerts[j];
//
//                    // (A) 속성 가져오기
//                    FVector Pos = (FVector)Mesh.GetVertex(VertID);
//
//                    FVector Normal = FVector::UpVector;
//                    if (NormalAttrib)
//                    {
//                        int32 ElemID = NormalAttrib->GetElementIDAtVertex(TriID, j);
//                        if (ElemID != -1) Normal = (FVector)NormalAttrib->GetElement(ElemID);
//                    }
//                    else if (bHasNormals)
//                    {
//                        Normal = (FVector)Mesh.GetVertexNormal(VertID);
//                    }
//
//
//                    FVector2D UV = FVector2D::ZeroVector;
//                    if (UVAttrib)
//                    {
//                        int32 ElemID = UVAttrib->GetElementIDAtVertex(TriID, j);
//                        if (ElemID != -1) UV = (FVector2D)UVAttrib->GetElement(ElemID);
//                    }
//
//                    FLinearColor VertColor = FLinearColor::White;
//                    //if (ColorAttrib)
//                    //{
//                    //    int32 ElemID = ColorAttrib->GetElementIDAtVertex(TriID, j);
//                    //    if (ElemID != -1) VertColor = FLinearColor(ColorAttrib->GetElement(ElemID));
//                    //}
//
//                    // (B) 버텍스 중복 검사 (위치 + UV + Normal + Color 비교)
//                    int32 FinalVertIndex = -1;
//
//                    if (Section.VertexMap.Contains(VertID))
//                    {
//                        const TArray<int32>& Candidates = Section.VertexMap[VertID];
//                        for (int32 ExistingIdx : Candidates)
//                        {
//                            // 속성이 유사하면(오차범위 내) 재사용
//                            if (Section.UVs[ExistingIdx].Equals(UV, 1e-4f) &&
//                                Section.Normals[ExistingIdx].Equals(Normal, 1e-4f) &&
//                                Section.Colors[ExistingIdx].Equals(VertColor, 1e-4f))
//                            {
//                                FinalVertIndex = ExistingIdx;
//                                break;
//                            }
//                        }
//                    }
//
//                    // (C) 새 버텍스 추가 (재사용 불가 시)
//                    if (FinalVertIndex == -1)
//                    {
//                        FinalVertIndex = Section.Vertices.Add(Pos);
//                        Section.Normals.Add(Normal);
//                        Section.UVs.Add(UV);
//                        Section.Colors.Add(VertColor);
//                        Section.Tangents.Add(FProcMeshTangent()); // 임시값
//
//                        Section.VertexMap.FindOrAdd(VertID).Add(FinalVertIndex);
//
//                        // 스킨 웨이트 캐싱
//                        FCachedSkinVertex CachedVert;
//                        CachedVert.InitialPos = Pos;
//                        CachedVert.InitialNormal = Normal;
//                        CachedVert.SectionIndex = MatID;
//                        CachedVert.VertIndex = FinalVertIndex;
//
//                        for (int k = 0; k < 4; k++) { CachedVert.BoneIndices[k] = 0; CachedVert.BoneWeights[k] = 0.f; }
//
//                        if (SkinWeightsAttr)
//                        {
//                            UE::AnimationCore::FBoneWeights Weights;
//                            SkinWeightsAttr->GetValue(VertID, Weights);
//                            int32 WriteIdx = 0;
//                            for (int32 k = 0; k < Weights.Num() && WriteIdx < 4; ++k)
//                            {
//                                const auto& BW = Weights[k];
//                                if (BW.GetWeight() > 0.001f)
//                                {
//                                    CachedVert.BoneIndices[WriteIdx] = BW.GetBoneIndex();
//                                    CachedVert.BoneWeights[WriteIdx] = BW.GetWeight();
//                                    WriteIdx++;
//                                }
//                            }
//                        }
//                        OutCache.Add(CachedVert);
//                    }
//
//                    // 삼각형 구성
//                    Section.Triangles.Add(FinalVertIndex);
//                }
//            }
//
//            // ---------------------------------------------------------------------
//            // 4. Procedural Mesh Section 생성
//            // ---------------------------------------------------------------------
//            for (auto& Elem : Sections)
//            {
//                int32 MatIndex = Elem.Key;
//                FMeshSectionData& Data = Elem.Value;
//
//                if (Data.Vertices.Num() > 0)
//                {
//                    // (A) 탄젠트 계산
//                    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
//                        Data.Vertices,
//                        Data.Triangles,
//                        Data.UVs,
//                        Data.Normals,
//                        Data.Tangents
//                    );
//
//                    // (B) [검은색 방지] 탄젠트가 0인 경우 강제 보정
//                    // UV가 뭉개져 있거나 없으면 Tangent가 (0,0,0)이 되어 빛 계산 시 검게 나옴
//                    for (int32 i = 0; i < Data.Tangents.Num(); ++i)
//                    {
//                        if (Data.Tangents[i].TangentX.IsNearlyZero())
//                        {
//                            // 노멀에 수직인 임의의 벡터 생성
//                            FVector ValidTangent = FVector::CrossProduct(Data.Normals[i], FVector::UpVector);
//                            if (ValidTangent.IsNearlyZero())
//                            {
//                                ValidTangent = FVector::CrossProduct(Data.Normals[i], FVector::RightVector);
//                            }
//                            Data.Tangents[i] = FProcMeshTangent(ValidTangent.GetSafeNormal(), false);
//                        }
//                    }
//
//                    // ========================= [DEBUG LOG START] =========================
//                    UE_LOG(LogTemp, Warning, TEXT("========== [SliceSystem] Mesh Data Inspection (Mat: %d) =========="), MatIndex);
//                    UE_LOG(LogTemp, Warning, TEXT("Counts -> Verts: %d | Normals: %d | Tangents: %d | UVs: %d | Colors: %d"),
//                        Data.Vertices.Num(), Data.Normals.Num(), Data.Tangents.Num(), Data.UVs.Num(), Data.Colors.Num());
//
//                    if (Data.Vertices.Num() > 0)
//                    {
//                        int32 ZeroNormals = 0;
//                        int32 ZeroTangents = 0;
//                        int32 ZeroUVs = 0;
//                        int32 BlackColors = 0; // Alpha가 0이거나 RGB가 모두 0인 경우
//
//                        for (int32 i = 0; i < Data.Vertices.Num(); ++i)
//                        {
//                            // 1. 노멀 검사
//                            bool bHasNormal = Data.Normals.IsValidIndex(i);
//                            if (bHasNormal && Data.Normals[i].IsNearlyZero()) ZeroNormals++;
//
//                            // 2. 탄젠트 검사
//                            bool bHasTangent = Data.Tangents.IsValidIndex(i);
//                            if (bHasTangent && Data.Tangents[i].TangentX.IsNearlyZero()) ZeroTangents++;
//
//                            // 3. UV 검사
//                            bool bHasUV = Data.UVs.IsValidIndex(i);
//                            if (bHasUV && Data.UVs[i].IsZero()) ZeroUVs++;
//
//                            // 4. 컬러 검사
//                            bool bHasColor = Data.Colors.IsValidIndex(i);
//                            if (bHasColor)
//                            {
//                                // 검은색이거나 투명하면 카운트
//                                if (Data.Colors[i].A <= 0.0f || (Data.Colors[i].R == 0.f && Data.Colors[i].G == 0.f && Data.Colors[i].B == 0.f))
//                                {
//                                    BlackColors++;
//                                }
//                            }
//
//                            // [상세 로그] 처음 5개만 찍어서 값 확인
//                            if (i < 5)
//                            {
//                                FString NStr = bHasNormal ? Data.Normals[i].ToString() : TEXT("None");
//                                FString TStr = bHasTangent ? Data.Tangents[i].TangentX.ToString() : TEXT("None");
//                                FString UVStr = bHasUV ? Data.UVs[i].ToString() : TEXT("None");
//                                FString CStr = bHasColor ? Data.Colors[i].ToString() : TEXT("None");
//
//                                UE_LOG(LogTemp, Display, TEXT("[%d] N:%s | T:%s | UV:%s | Color:%s"), i, *NStr, *TStr, *UVStr, *CStr);
//                            }
//                        }
//
//                        UE_LOG(LogTemp, Error, TEXT(">>> SUMMARY: ZeroNormals: %d, ZeroTangents: %d, ZeroUVs: %d, BlackColors: %d <<<"),
//                            ZeroNormals, ZeroTangents, ZeroUVs, BlackColors);
//
//                        if (ZeroTangents > 0) UE_LOG(LogTemp, Error, TEXT("!!! WARNING: Tangents are ZERO. Light calculation will fail (Black Mesh). !!!"));
//                        if (BlackColors == Data.Vertices.Num()) UE_LOG(LogTemp, Error, TEXT("!!! WARNING: All Colors are BLACK/TRANSPARENT. Vertex Color Multiply will make it Black. !!!"));
//                    }
//                    // ========================= [DEBUG LOG END] =========================
//
//                    // (C) 섹션 생성
//                    ProcMeshComp->CreateMeshSection_LinearColor(
//                        MatIndex,
//                        Data.Vertices,
//                        Data.Triangles,
//                        Data.Normals,
//                        Data.UVs,
//                        {}, {}, {}, // Extra UVs
//                        Data.Colors,
//                        Data.Tangents,
//                        true // Collision Enable
//                    );
//
//                    // (D) 머터리얼 연결
//                    UMaterialInterface* TargetMat = nullptr;
//                    if (MatIndex >= 0 && MatIndex < DynamicMeshComp->GetNumMaterials())
//                    {
//                        TargetMat = DynamicMeshComp->GetMaterial(MatIndex);
//                    }
//
//                    // 머터리얼이 없으면 0번이라도 가져오기 (Fallback)
//                    if (!TargetMat && DynamicMeshComp->GetNumMaterials() > 0)
//                    {
//                        TargetMat = DynamicMeshComp->GetMaterial(0);
//                    }
//
//                    if (TargetMat)
//                    {
//                        ProcMeshComp->SetMaterial(MatIndex, TargetMat);
//                    }
//                }
//            }
//        });
//}
//void USliceUtils::ConvertDynamicMeshToProcMesh(UDynamicMeshComponent* DynamicMeshComp, UProceduralMeshComponent* ProcMeshComp, TArray<FCachedSkinVertex>& OutCache, const TSet<int32>& AllowedBoneIndices)
//{
//    if (!DynamicMeshComp || !ProcMeshComp) return;
//
//    ProcMeshComp->ClearAllMeshSections();
//    OutCache.Reset();
//
//    UDynamicMesh* DynMesh = DynamicMeshComp->GetDynamicMesh();
//    if (!DynMesh) return;
//
//    DynMesh->ProcessMesh([&](const UE::Geometry::FDynamicMesh3& Mesh)
//        {
//            // 1. 스킨 웨이트 속성 가져오기
//            const auto* SkinWeightsAttr = Mesh.Attributes()->GetSkinWeightsAttribute(FSkeletalMeshAttributes::DefaultSkinWeightProfileName);
//
//            // 데이터 저장 구조체
//            struct FMeshSectionData
//            {
//                TArray<FVector> Vertices;
//                TArray<int32> Triangles;
//                TArray<FVector> Normals;
//                TArray<FVector2D> UVs;
//                TArray<FProcMeshTangent> Tangents;
//                TArray<FLinearColor> Colors;
//
//                // VertID 하나에 여러 개의 PMC Index가 매핑될 수 있도록 변경
//                TMap<int32, TArray<int32>> VertexMap;
//            };
//            TMap<int32, FMeshSectionData> Sections;
//
//            // 속성 접근자
//            bool bHasMaterials = Mesh.HasAttributes() && Mesh.Attributes()->HasMaterialID();
//            const auto* MaterialIDAttrib = bHasMaterials ? Mesh.Attributes()->GetMaterialID() : nullptr;
//
//            // 오버레이 노멀 사용 (GeometryScript 결과물은 보통 오버레이 노멀임)
//            const auto* NormalAttrib = Mesh.HasAttributes() ? Mesh.Attributes()->PrimaryNormals() : nullptr;
//            bool bHasNormals = Mesh.HasVertexNormals(); // Fallback
//
//            bool bHasUVs = Mesh.HasAttributes() && Mesh.Attributes()->NumUVLayers() > 0;
//            const auto* UVAttrib = bHasUVs ? Mesh.Attributes()->GetUVLayer(0) : nullptr;
//
//            bool bHasTangents = Mesh.HasAttributes() && Mesh.Attributes()->HasTangentSpace();
//            const auto* TangentAttrib = bHasTangents ? Mesh.Attributes()->PrimaryTangents() : nullptr;
//
//            //버텍스 컬러 속성 접근자
//            bool bHasColors = Mesh.HasAttributes() && Mesh.Attributes()->HasPrimaryColors();
//            const auto* ColorAttrib = bHasColors ? Mesh.Attributes()->PrimaryColors() : nullptr;
//
//            // ---------------------------------------------------------------------
//            // 삼각형 순회 및 필터링
//            // ---------------------------------------------------------------------
//            for (int32 TriID : Mesh.TriangleIndicesItr())
//            {
//                UE::Geometry::FIndex3i TriVerts = Mesh.GetTriangle(TriID);
//
//                // 이 삼각형을 렌더링할지 결정하는 플래그
//                bool bKeepTriangle = false;
//
//                // 1. 머터리얼 ID 확인 (단면인지 1차 확인)
//                int32 MatID = 0;
//                if (MaterialIDAttrib) MatID = MaterialIDAttrib->GetValue(TriID);
//
//                // 2. 삼각형 구성 버텍스들의 웨이트 검사
//                //    (삼각형 중 하나라도 조건에 맞으면 그 삼각형은 그려야 함)
//                if (SkinWeightsAttr)
//                {
//                    for (int32 j = 0; j < 3; j++)
//                    {
//                        int32 VertID = TriVerts[j];
//                        UE::AnimationCore::FBoneWeights Weights;
//                        SkinWeightsAttr->GetValue(VertID, Weights);
//
//                        float TotalWeight = 0.f;
//                        bool bHasAllowedBone = false;
//
//                        for (const auto& BW : Weights)
//                        {
//                            float W = BW.GetWeight();
//                            if (W > 0.001f) // 유의미한 웨이트만
//                            {
//                                TotalWeight += W;
//                                // 허용된 본 목록에 있는 경우
//                                if (AllowedBoneIndices.Contains(BW.GetBoneIndex()))
//                                {
//                                    bHasAllowedBone = true;
//                                }
//                            }
//                        }
//
//                        // [조건 A] 웨이트 합이 0이다 -> 단면(Cap) 버텍스 -> 무조건 생성
//                        if (TotalWeight < 0.001f)
//                        {
//                            bKeepTriangle = true;
//                            break;
//                        }
//
//                        // [조건 B] 허용된 본의 웨이트를 가지고 있다 -> 생성
//                        if (bHasAllowedBone)
//                        {
//                            bKeepTriangle = true;
//                            break;
//                        }
//                    }
//                }
//                else
//                {
//                    // 웨이트 속성이 아예 없으면 (Static Mesh 등) 그냥 다 그린다.
//                    bKeepTriangle = true;
//                }
//
//                // 조건에 맞지 않는 삼각형(다른 부위의 본만 가진 삼각형)은 건너뜀
//                if (!bKeepTriangle) continue;
//
//
//                // -----------------------------------------------------------------
//                // 데이터 추출 및 섹션 추가 (기존 로직과 동일)
//                // -----------------------------------------------------------------
//                FMeshSectionData& Section = Sections.FindOrAdd(MatID);
//
//                for (int32 j = 0; j < 3; j++)
//                {
//                    int32 VertID = TriVerts[j];
//
//                    // --- 속성 추출 ---
//                    FVector Pos = (FVector)Mesh.GetVertex(VertID);
//
//                    FVector Normal = FVector::UpVector;
//                    if (NormalAttrib)
//                    {
//                        int32 ElemID = NormalAttrib->GetElementIDAtVertex(TriID, j);
//                        if (ElemID != -1) Normal = (FVector)NormalAttrib->GetElement(ElemID);
//                    }
//                    else if (bHasNormals)
//                    {
//                        Normal = (FVector)Mesh.GetVertexNormal(VertID);
//                    }
//
//                    FVector2D UV = FVector2D::ZeroVector;
//                    if (UVAttrib)
//                    {
//                        int32 ElemID = UVAttrib->GetElementIDAtVertex(TriID, j);
//                        if (ElemID != -1) UV = (FVector2D)UVAttrib->GetElement(ElemID);
//                    }
//
//                    FLinearColor VertColor = FLinearColor::White;
//                    if (ColorAttrib)
//                    {
//                        int32 ElemID = ColorAttrib->GetElementIDAtVertex(TriID, j);
//                        if (ElemID != -1) VertColor = FLinearColor(ColorAttrib->GetElement(ElemID));
//                    }
//
//                    // --- [수정된 버텍스 병합 로직] ---
//                    // 위치 ID(VertID)가 같더라도 UV나 Normal이 다르면 분리해야 함
//                    int32 FinalVertIndex = -1;
//
//                    if (Section.VertexMap.Contains(VertID))
//                    {
//                        const TArray<int32>& Candidates = Section.VertexMap[VertID];
//                        for (int32 ExistingIdx : Candidates)
//                        {
//                            // 이미 등록된 버텍스와 속성이 유사한지 검사 (오차 허용)
//                            if (Section.UVs[ExistingIdx].Equals(UV, 1e-4f) &&
//                                Section.Normals[ExistingIdx].Equals(Normal, 1e-4f) &&
//                                Section.Colors[ExistingIdx].Equals(VertColor, 1e-4f))
//                            {
//                                FinalVertIndex = ExistingIdx;
//                                break;
//                            }
//                        }
//                    }
//
//                    // 재사용할 버텍스를 찾지 못했으면 새로 추가
//                    if (FinalVertIndex == -1)
//                    {
//                        FinalVertIndex = Section.Vertices.Add(Pos);
//                        Section.Normals.Add(Normal);
//                        Section.UVs.Add(UV);
//                        Section.Colors.Add(VertColor);
//                        Section.Tangents.Add(FProcMeshTangent()); // 임시 값, 나중에 계산됨
//
//                        // 맵에 등록 (리스트에 추가)
//                        Section.VertexMap.FindOrAdd(VertID).Add(FinalVertIndex);
//
//                        // --- 스킨 웨이트 캐싱 (새 버텍스일 때만 수행) ---
//                        FCachedSkinVertex CachedVert;
//                        CachedVert.InitialPos = Pos;
//                        CachedVert.InitialNormal = Normal;
//                        CachedVert.SectionIndex = MatID;
//                        CachedVert.VertIndex = FinalVertIndex;
//
//                        // 초기화
//                        for (int k = 0; k < 4; k++) { CachedVert.BoneIndices[k] = 0; CachedVert.BoneWeights[k] = 0.f; }
//
//                        if (SkinWeightsAttr)
//                        {
//                            UE::AnimationCore::FBoneWeights Weights;
//                            SkinWeightsAttr->GetValue(VertID, Weights);
//
//                            int32 WriteIdx = 0;
//                            for (int32 k = 0; k < Weights.Num() && WriteIdx < 4; ++k)
//                            {
//                                const auto& BW = Weights[k];
//                                // 0에 가까운 웨이트는 제외하여 불필요한 연산 방지
//                                if (BW.GetWeight() > 0.001f)
//                                {
//                                    CachedVert.BoneIndices[WriteIdx] = BW.GetBoneIndex();
//                                    CachedVert.BoneWeights[WriteIdx] = BW.GetWeight();
//                                    WriteIdx++;
//                                }
//                            }
//                        }
//                        OutCache.Add(CachedVert);
//                    }
//                    // 삼각형 인덱스 추가
//                    Section.Triangles.Add(FinalVertIndex);
//                }
//            }
//
//            // 3. PMC 생성
//            for (auto& Elem : Sections)
//            {
//                int32 MatIndex = Elem.Key;
//                FMeshSectionData& Data = Elem.Value;
//
//                if (Data.Vertices.Num() > 0)
//                {
//                    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
//                        Data.Vertices,
//                        Data.Triangles,
//                        Data.UVs,
//                        Data.Normals,
//                        Data.Tangents // 여기에 올바른 값이 채워짐
//                    );
//
//                    ProcMeshComp->CreateMeshSection_LinearColor(
//                        MatIndex,
//                        Data.Vertices,
//                        Data.Triangles,
//                        Data.Normals,
//                        Data.UVs,
//                        {}, {}, {}, // UV 1~3
//                        Data.Colors,
//                        Data.Tangents,
//                        true // Collision
//                    );
//
//                    UMaterialInterface* TargetMat = nullptr;
//                    if (MatIndex < DynamicMeshComp->GetNumMaterials())
//                    {
//                        TargetMat = DynamicMeshComp->GetMaterial(MatIndex);
//                    }
//
//                    // DMC에 없으면 0번이라도 가져오기 (비상용)
//                    if (!TargetMat) TargetMat = DynamicMeshComp->GetMaterial(0);
//
//                    if (TargetMat)
//                    {
//                        ProcMeshComp->SetMaterial(MatIndex, TargetMat);
//                    }
//                }
//            }
//        });
//}

void USliceUtils::InitializeDMCFromSkeletalMesh(UDynamicMeshComponent* DMC, USkeletalMeshComponent* SkelMeshComp, EGeometryScriptOutcomePins& OutCome)
{

    OutCome = EGeometryScriptOutcomePins::Failure;

    if (!DMC || !SkelMeshComp)
    {
        UE_LOG(LogTemp, Error, TEXT("InitializeDMCFromSkeletalMesh: Invalid Components"));
        return;
    }

    USkeletalMesh* SkelAsset = SkelMeshComp->GetSkeletalMeshAsset();
    if (!SkelAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("InitializeDMCFromSkeletalMesh: SkeletalMeshAsset is Null"));
        return;
    }

    // 3. RenderData 가져오기 (Allow CPU Access 체크 필수)
    FSkeletalMeshRenderData* RenderData = SkelAsset->GetResourceForRendering();
    if (!RenderData || !RenderData->LODRenderData.IsValidIndex(0))
    {
        UE_LOG(LogTemp, Error, TEXT("InitializeDMCFromSkeletalMesh: RenderData invalid. Check 'Allow CPU Access' in Asset!"));
        return;
    }

    UDynamicMesh* DynMesh = DMC->GetDynamicMesh();

    // 내부 변환 성공 여부를 확인하기 위한 플래그
    bool bConversionSuccess = false;

    // 4. EditMesh로 안전하게 메쉬 수정 진입
    // [&] 캡처를 통해 외부의 bConversionSuccess 변수에 접근합니다.
    DynMesh->EditMesh([&](UE::Geometry::FDynamicMesh3& OutMesh)
        {
            OutMesh.Clear();

            const FSkeletalMeshLODRenderData* LODData = &RenderData->LODRenderData[0];
            const FReferenceSkeleton& RefSkeleton = SkelAsset->GetRefSkeleton();

            UE::Geometry::FSkeletalMeshLODRenderDataToDynamicMesh::ConversionOptions Options;

            Options.bWantSkinWeights = true;  // <--- 핵심!
            Options.bWantNormals = true;
            Options.bWantUVs = true;
            Options.bWantTangents = true;

            // Options.BuildScale = (FVector3d)SkelMeshComp->GetComponentScale();

            // Convert 함수가 true를 반환해야 성공한 것입니다.
            if (UE::Geometry::FSkeletalMeshLODRenderDataToDynamicMesh::Convert(
                LODData,
                RefSkeleton,
                Options,
                OutMesh
            ))
            {
                bConversionSuccess = true;
                // 속성 안전장치
                OutMesh.EnableAttributes();
                
                FName ProfileName = FSkeletalMeshAttributes::DefaultSkinWeightProfileName;
                UE_LOG(LogTemp, Log, TEXT("SkinWeights with name '%s'"), *ProfileName.ToString());
                if (OutMesh.Attributes()->HasSkinWeightsAttribute(ProfileName))
                {
                    
                }
                else
                {
                    // 3. 없다면, 도대체 무슨 이름으로 들어있는지(혹은 아예 없는지) 전수 조사
                    bConversionSuccess = false;
                    UE_LOG(LogTemp, Error, TEXT("Error: Convert success but 'SkinWeights' attribute is missing!"));  
                }
            }
            else

            {
                UE_LOG(LogTemp, Error, TEXT("InitializeDMCFromSkeletalMesh: Conversion Failed inside EditMesh"));
            }

        }, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::Unknown, true);

    // 6. 결과 처리
    if (bConversionSuccess)
    {
        DMC->NotifyMeshUpdated();
        OutCome = EGeometryScriptOutcomePins::Success;
        UE_LOG(LogTemp, Log, TEXT("InitializeDMCFromSkeletalMesh: Success"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("InitializeDMCFromSkeletalMesh: Failed to initialize mesh"));
    }
}

void USliceUtils::ApplySkinningWithDMCData(UDynamicMeshComponent* DMC, USkeletalMeshComponent* SkelMeshComp)
{
    // 1. 유효성 체크
    if (!DMC || !SkelMeshComp)
    {
        UE_LOG(LogTemp, Log, TEXT("Error: DMC or SkelMeshComp is Null"));
        return;
    }

    USkeletalMesh* SkelAsset = SkelMeshComp->GetSkeletalMeshAsset();
    if (!SkelAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("Error: SkelMeshAsset is Null"));
        return;
    }

    UDynamicMesh* DynMesh = DMC->GetDynamicMesh();
    if (!DynMesh)
    {
        UE_LOG(LogTemp, Warning, (TEXT("Error: DynamicMesh is Null")));
        return;
    }

    UE_LOG(LogTemp, Warning, (TEXT("ApplySkinningWithDMCData: Start")));

    // 2. 데이터 준비
    const TArray<FTransform>& CurrentBoneTransforms = SkelMeshComp->GetComponentSpaceTransforms();

    // UE5.0+ GetRefBasesInvMatrix()
    const TArray<FMatrix44f>& RefInvMatrices = SkelAsset->GetRefBasesInvMatrix();

    int32 NumBones = RefInvMatrices.Num();
    int32 NumCurrentTransforms = CurrentBoneTransforms.Num();

    // 디버그: 본 개수 확인
    UE_LOG(LogTemp, Warning, TEXT("Bone Check - RefBones: %d, CurrentTransforms: %d"), NumBones, NumCurrentTransforms);

    if (NumCurrentTransforms < NumBones)
    {
        UE_LOG(LogTemp, Warning, TEXT("Error: Bone count mismatch! Transforms are fewer than RefBones."));
        return;
    }

    // 3. 스키닝 행렬 미리 계산
    TArray<FMatrix> SkinningMatrices;
    SkinningMatrices.SetNumUninitialized(NumBones);

    for (int32 i = 0; i < NumBones; i++)
    {
        FMatrix RefInvMat = FMatrix(RefInvMatrices[i]);
        FMatrix CurrentMat = CurrentBoneTransforms[i].ToMatrixWithScale();
        SkinningMatrices[i] = RefInvMat * CurrentMat;
    }

    FTransform DMCToWorld = DMC->GetComponentTransform();
    FTransform SkelToWorld = SkelMeshComp->GetComponentTransform();
    FMatrix ComponentToDMCLocal = (SkelToWorld * DMCToWorld.Inverse()).ToMatrixWithScale();

    // 디버그: DMC와 SkelMesh의 위치 차이 확인
    UE_LOG(LogTemp, Warning, TEXT("Transform Check - SkelLoc: %s, DMCLoc: %s"), *SkelToWorld.GetLocation().ToString(), *DMCToWorld.GetLocation().ToString());


    // 4. 메쉬 편집 (CPU Skinning)
    DynMesh->EditMesh([&](UE::Geometry::FDynamicMesh3& Mesh)
        {
            // 디버그: 속성 확인
            if (!Mesh.HasAttributes())
            {
                UE_LOG(LogTemp, Warning, TEXT("Error: Mesh has no attributes set!"));
                return;
            }
            FName ProfileName = FSkeletalMeshAttributes::DefaultSkinWeightProfileName;
            if (!Mesh.Attributes()->HasSkinWeightsAttribute(ProfileName))
            {
                UE_LOG(LogTemp, Warning, TEXT("Error: Mesh has no 'SkinWeights' attribute!"));
                return;
            }

            const auto* SkinWeightsAttr = Mesh.Attributes()->GetSkinWeightsAttribute(ProfileName);

            TArray<int32> BoneIndices;
            TArray<float> BoneWeights;

            int32 ProcessedVerts = 0;
            int32 MovedVerts = 0;

            // 버텍스 루프
            for (int32 VertID : Mesh.VertexIndicesItr())
            {
                FVector OriginalPos = Mesh.GetVertex(VertID);
                FVector FinalPosCompSpace = FVector::ZeroVector;

                SkinWeightsAttr->GetValue(VertID, BoneIndices, BoneWeights);

                float TotalWeight = 0.0f;

                for (int32 i = 0; i < BoneWeights.Num(); ++i)
                {
                    int32 BoneIndex = BoneIndices[i];
                    float Weight = BoneWeights[i];

                    if (Weight < KINDA_SMALL_NUMBER) continue;

                    if (SkinningMatrices.IsValidIndex(BoneIndex))
                    {
                        FinalPosCompSpace += SkinningMatrices[BoneIndex].TransformPosition(OriginalPos) * Weight;
                        TotalWeight += Weight;
                    }
                }

                // 디버그: 첫 번째 버텍스(ID 0)에 대해서만 상세 로그 출력 (모두 출력하면 너무 느려짐)
                if (VertID == 0)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Debug Vert[0] - Org: %s, SkinSpace: %s, TotalWeight: %f, NumBonesInf: %d"),
                        *OriginalPos.ToString(),
                        *FinalPosCompSpace.ToString(),
                        TotalWeight,
                        BoneWeights.Num());
                }

                if (TotalWeight > 0.0f)
                {
                    FVector FinalPosLocal = ComponentToDMCLocal.TransformPosition(FinalPosCompSpace);
                    Mesh.SetVertex(VertID, FinalPosLocal);
                    MovedVerts++;
                }

                ProcessedVerts++;
            }

            // 디버그: 전체 처리 통계
            UE_LOG(LogTemp, Warning, TEXT("Skinning Loop Done - TotalVerts: %d, MovedVerts: %d"), ProcessedVerts, MovedVerts);

        }, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::VertexPositions, false);

    // 5. 노멀 재계산 (주석 해제 권장)
    // UE::Geometry::FMeshNormals::QuickComputeVertexNormals(*DynMesh->GetMeshPtr());

    DMC->NotifyMeshUpdated();

    UE_LOG(LogTemp, Warning, TEXT("ApplySkinningWithDMCData: Finished"));
}

void USliceUtils::CloseMeshHoles(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FProcMeshTangent>& Tangents, TArray<FLinearColor>& Colors)
{
    // 1. 엣지 사용 횟수 카운팅
    TMap<FMeshEdge, int32> EdgeCountMap;

    for (int32 i = 0; i < Triangles.Num(); i += 3)
    {
        int32 I0 = Triangles[i];
        int32 I1 = Triangles[i + 1];
        int32 I2 = Triangles[i + 2];

        FMeshEdge E1(I0, I1);
        FMeshEdge E2(I1, I2);
        FMeshEdge E3(I2, I0);

        EdgeCountMap.FindOrAdd(E1)++;
        EdgeCountMap.FindOrAdd(E2)++;
        EdgeCountMap.FindOrAdd(E3)++;
    }

    // 2. 경계선 엣지(Boundary Edges) 추출
    // 사용 횟수가 1인 엣지가 구멍의 테두리입니다.
    TArray<FMeshEdge> BoundaryEdges;
    for (const auto& Pair : EdgeCountMap)
    {
        if (Pair.Value == 1) // 공유되지 않은 엣지
        {
            BoundaryEdges.Add(Pair.Key);
        }
    }

    if (BoundaryEdges.Num() == 0) return; // 이미 닫힌 도형임

    // 3. 루프(Loop)별로 그룹화 및 캡 생성
    // (간단한 구현을 위해 모든 경계선 엣지의 중심점을 구해 부채꼴로 막습니다)
    // * 복잡한 형상은 루프 분리가 필요하지만, 팔다리 같은 원통형은 중심점 방식으로 대부분 해결됨 *

    // 경계선 버텍스들의 평균 위치(Centroid) 계산
    FVector Centroid = FVector::ZeroVector;
    TSet<int32> BoundaryVerts;

    for (const FMeshEdge& Edge : BoundaryEdges)
    {
        BoundaryVerts.Add(Edge.VertA);
        BoundaryVerts.Add(Edge.VertB);
    }

    for (int32 VertIdx : BoundaryVerts)
    {
        Centroid += Vertices[VertIdx];
    }
    Centroid /= BoundaryVerts.Num();

    // 4. 중심점 버텍스 추가
    int32 CenterIndex = Vertices.Add(Centroid);

    // 더미 데이터 채우기 (나중에 CalculateTangentsForMesh로 덮어씌워짐)
    Normals.Add(FVector::UpVector);
    UVs.Add(FVector2D::ZeroVector);
    Tangents.Add(FProcMeshTangent(FVector::RightVector, false));
    Colors.Add(FLinearColor::Black); // 캡 부분은 검은색 처리 (디버깅용)

    // 5. 경계선 엣지와 중심점을 연결해 삼각형 생성
    for (const FMeshEdge& Edge : BoundaryEdges)
    {
        // 엣지의 방향성을 맞춰야 함 (Winding Order)
        // 기존 삼각형에서 Edge가 어떻게 쓰였는지 확인해야 법선 방향이 맞음
        // 하지만 여기서는 간단히 양면 렌더링을 가정하거나, Slice 함수가 알아서 처리하게 둡니다.
        // SliceProceduralMesh는 닫힌 부피만 있으면 내부를 인식합니다.

        Triangles.Add(Edge.VertA);
        Triangles.Add(Edge.VertB);
        Triangles.Add(CenterIndex);

        // 뒷면도 추가 (확실하게 막기 위해 양면 생성)
        Triangles.Add(Edge.VertB);
        Triangles.Add(Edge.VertA);
        Triangles.Add(CenterIndex);
    }
}

void USliceUtils::FindPlaneCutBones(USkeletalMesh* SkeletalMesh, FTransform& PlaneTransform, TSet<int32>& OutBoneIndices)
{

    // T-Pose 상태의 모든 뼈를 순회하며 절단면과 교차하는지 검사

    // 1. 평면 식 생성 (위치, 노멀)
    FVector PlanePos = PlaneTransform.GetLocation();
    FVector PlaneNormal = PlaneTransform.GetUnitAxis(EAxis::Z);
    FPlane CutPlane(PlanePos, PlaneNormal);

    // 2. 전체 뼈 순회 (루트 제외)
    const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
    TArray<FTransform> RefComponentSpaceTransforms;
    FAnimationRuntime::FillUpComponentSpaceTransforms(RefSkeleton, RefSkeleton.GetRefBonePose(), RefComponentSpaceTransforms);

    for (int32 i = 1; i < RefSkeleton.GetNum(); ++i)
    {
        int32 ParentIndex = RefSkeleton.GetParentIndex(i);
        if (ParentIndex == INDEX_NONE) continue;

        // 부모 뼈 위치(Start)와 내 뼈 위치(End)가 평면을 통과하는지 확인
        FVector BoneStart = RefComponentSpaceTransforms[ParentIndex].GetLocation();
        FVector BoneEnd = RefComponentSpaceTransforms[i].GetLocation();

        // 교차 검사
        FVector IntersectionPoint;
        if (FMath::SegmentPlaneIntersection(BoneStart, BoneEnd, CutPlane, IntersectionPoint))
        {
            OutBoneIndices.Add(i);
            UE_LOG(LogTemp, Warning, TEXT("Intersected Bone: %s"), *RefSkeleton.GetBoneName(i).ToString());
        }
    }
}
