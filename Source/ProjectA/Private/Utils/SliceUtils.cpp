// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/SliceUtils.h"
#include "ProceduralMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshRenderData.h" 
#include "Rendering/PositionVertexBuffer.h" // 위치 데이터 접근용
#include "Rendering/StaticMeshVertexBuffer.h" // 노멀/UV 데이터 접근용

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

    // [수정 1] 타겟 본의 Reference Pose(Bind Pose) Component Transform 계산
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
                        // [수정 2] 좌표 및 벡터 변환 적용
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

            // [핵심 수정] 머터리얼 할당 로직
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
            // [핵심 로직] "가장 영향력이 큰 본(Dominant Bone)" 찾기
            // ---------------------------------------------------------
            int32 BestBoneIndex = INDEX_NONE;
            float MaxWeight = -1.0f;

            int32 NumInfluences = SkinWeightBuffer.GetMaxBoneInfluences();
            for (int32 InfIdx = 0; InfIdx < NumInfluences; InfIdx++)
            {
                float Weight = SkinWeightBuffer.GetBoneWeight(VertexIndex, InfIdx);
                if (Weight > MaxWeight)
                {
                    int32 BoneIndexInBuffer = SkinWeightBuffer.GetBoneIndex(VertexIndex, InfIdx);

                    // BoneMap 변환
                    int32 RealBoneIndex = BoneIndexInBuffer;
                    if (Section.BoneMap.Num() > 0 && Section.BoneMap.IsValidIndex(BoneIndexInBuffer))
                    {
                        RealBoneIndex = Section.BoneMap[BoneIndexInBuffer];
                    }

                    MaxWeight = Weight;
                    BestBoneIndex = RealBoneIndex;
                }
            }

            // 가장 큰 영향을 주는 본이 "타겟 본"일 때만 숨김
            // 이렇게 해야 LowerArm(자식)과 연결된 관절 부위가 안전하게 보존됨
            if (BestBoneIndex == TargetBoneIndex)
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
