// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/SliceUtils.h"
#include "ProceduralMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshRenderData.h" 
#include "Rendering/PositionVertexBuffer.h" // 위치 데이터 접근용
#include "Rendering/StaticMeshVertexBuffer.h" // 노멀/UV 데이터 접근용
#include "KismetProceduralMeshLibrary.h"

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

                // 웨이트가 0.01보다 작으면 무시 (영향력이 거의 없는 경우)
                if (Weight < 0.01f)
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
