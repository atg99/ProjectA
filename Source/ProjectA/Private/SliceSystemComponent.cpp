// Fill out your copyright notice in the Description page of Project Settings.


#include "SliceSystemComponent.h"
#include "GameFramework/Character.h"
#include "Utils/SliceUtils.h"
#include "KismetProceduralMeshLibrary.h"
#include "ProceduralMeshComponent.h"
#include "Utils/NetworkUtil.h"

// 지오메트리 스크립트
#include "Components/DynamicMeshComponent.h"
#include "GeometryScript/MeshAssetFunctions.h"
#include "GeometryScript/MeshBoneWeightFunctions.h" 
#include "GeometryScript/MeshBooleanFunctions.h"   
#include "GeometryScript/MeshRepairFunctions.h"     
#include "GeometryScript/SceneUtilityFunctions.h"
#include "GeometryScript/MeshNormalsFunctions.h"
#include "GeometryScript/MeshBakeFunctions.h"

#include "Kismet/KismetMathLibrary.h" 
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "Rendering/SkeletalMeshRenderData.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/MeshAttributeUtil.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/DynamicVertexSkinWeightsAttribute.h"
#include "Operations/TransferBoneWeights.h"

// Sets default values for this component's properties
USliceSystemComponent::USliceSystemComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void USliceSystemComponent::BeginPlay()
{
	Super::BeginPlay();

    SetupPMCs();
   
    SetupDMCs();

    if (!IsRunningDedicatedServer())
    {
        if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
        {
            if (USkeletalMesh* OriginalMesh = Character->GetMesh()->GetSkeletalMeshAsset())
            {
                // 원본 에셋을 복제 독립적인 에셋 생성
                USkeletalMesh* NewMesh = DuplicateObject<USkeletalMesh>(OriginalMesh, Character);
                Character->GetMesh()->SetSkeletalMeshAsset(NewMesh);
            }
        }
    }
}

// Called every frame
void USliceSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void USliceSystemComponent::SliceBone(FName TargetBone, const FVector& HitLocation, const FVector& HitNormal, const FVector& CutNormal, float ImpulsePower)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;

    USkeletalMeshComponent* Mesh = Character->GetMesh();

    // 1. 유틸 함수 호출 (이제 PMC_Stump는 TargetBone의 로컬 좌표계로 데이터가 채워짐)
    USliceUtils::ConvertBoneToProcMesh(Mesh, TargetBone, PMC_Stump);

    // [중요] 자르기 전에 'Stump'를 타겟 본에 먼저 붙여야, Slice 함수가 월드 좌표를 로컬로 올바르게 변환함
    // 또한 애니메이션 위치에 맞게 메시가 정렬됨
    PMC_Stump->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetBone);
    PMC_Stump->SetRelativeTransform(FTransform::Identity); // 로컬 좌표계이므로 0,0,0으로 초기화
    PMC_Stump->UpdateComponentToWorld(); // 월드 좌표 갱신 보장

    // 2. 자르기
    // PMC_Stump가 이미 본 위치에 가 있으므로, Slice 함수가 내부적으로 HitLocation을 로컬로 잘 변환함
    FVector CutLocation = PMC_Stump->Bounds.Origin;

	UMaterialInterface* CapMaterial = SliceCapMaterial ? SliceCapMaterial : Mesh->GetMaterial(0);
    UKismetProceduralMeshLibrary::SliceProceduralMesh(
        PMC_Stump, HitLocation, FVector::UpVector, true,
        PMC_Debris, EProcMeshSliceCapOption::CreateNewSectionForCap,
        CapMaterial
    );

    //// 3. 위치 재조정 및 부착

    // [Debris 설정] : 잘려나간 부위
    // Debris는 TargetBone(물리 시뮬레이션 될 본)에 붙입니다.
    PMC_Debris->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetBone);
    PMC_Debris->SetRelativeTransform(FTransform::Identity); // 오프셋 없이 본 위치에 일치시킴
   /* PMC_Stump->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, false));
    PMC_Stump->SetWorldTransform(FTransform(PMC_Stump->GetComponentRotation(), PMC_Stump->GetComponentLocation() + FVector(0.f, 0.f, 15.f), FVector::OneVector));*/
     
    // [Stump 설정] : 몸통에 붙어있는 부위
    // Stump는 ParentBone에 붙어야 몸통을 따라다닙니다.
    // 하지만 Stump의 데이터는 TargetBone 좌표계 기준이므로, Parent에 그냥 붙이면 위치가 어긋납니다.
    // ParentBone 기준으로 TargetBone이 어디에 있는지(Ref Pose 오프셋)를 설정해줘야 합니다.

    FName ParentBone = Mesh->GetParentBone(TargetBone);
    int32 TargetBoneIndex = Mesh->GetBoneIndex(TargetBone);

    if (TargetBoneIndex != INDEX_NONE && ParentBone != NAME_None)
    {
        // 1. 부모 본에 부착
        PMC_Stump->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ParentBone);

        // 2. 오프셋 적용 (TargetBone의 로컬 Transform을 적용)
        FTransform RefBoneTransform = Mesh->GetSkeletalMeshAsset()->GetRefSkeleton().GetRefBonePose()[TargetBoneIndex];
        PMC_Stump->SetRelativeTransform(RefBoneTransform);
    }
    else
    {
        // 부모가 없는 루트 본이거나 오류 상황이면 그냥 둠
        PMC_Stump->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetBone);
    }

    ////단면 메쉬 덮기
    //if (MeatCrossSectionMesh)
    //{
    //    // 캡 생성 헬퍼 람다 함수
    //    auto AddCapMesh = [&](USceneComponent* ParentComp, FVector Location, FVector Normal)
    //        {
    //            UStaticMeshComponent* CapComp = NewObject<UStaticMeshComponent>(Character);
    //            CapComp->SetStaticMesh(MeatCrossSectionMesh);
    //            CapComp->RegisterComponent();
    //            
    //            FVector DebrisCenter = PMC_Debris->Bounds.Origin;
    //            FVector DebrisExtents = PMC_Debris->Bounds.BoxExtent;
    //            FVector CapLocation = FVector::PointPlaneProject(DebrisCenter, FPlane(Location, Normal));
    //            // 1) 위치 설정: 절단 위치
    //            CapComp->SetWorldLocation(CapLocation);

    //            // 2) 회전 설정: Normal 방향을 바라보게 함 (메시의 Forward가 X축이라고 가정)
    //            CapComp->SetWorldRotation(UKismetMathLibrary::MakeRotFromZ(Normal));

    //            //TArray<double> Sizes = { DebrisExtents.X, DebrisExtents.Y, DebrisExtents.Z };
    //            //Sizes.Sort(); // 작은 순서대로 정렬
    //            //double CrossSectionRadius = (Sizes[0] + Sizes[1]) * 0.5f; // 가장 작은 두 축의 평균 반지름

    //            //// 메시 기본 크기(100단위) 가정시 0.01 곱함. 
    //            //// 꽉 채우기 위해 1.5배 정도 더 크게 설정 (오버사이즈가 구멍보다 낫습니다)
    //            //double FinalScale = (CrossSectionRadius * 2.0f * 0.01f) * 1.2f;

    //            // 1. 피직스 에셋에서 뼈의 실제 굵기(반지름) 가져오기
    //            float BoneRadius = GetBoneRadius(Mesh, TargetBone);

    //            // 2. 스케일 적용 (메시가 100단위(1m)라고 가정할 때 0.01 곱함 + 여유분 1.1배)
    //            float FinalScale = (BoneRadius * 2.0f * 0.01f) * 1.1f;

    //            // 납작한 고기 단면이므로 X(두께)는 얇게, YZ(단면)는 넓게
    //            CapComp->SetWorldScale3D(FVector(0.2f, FinalScale, FinalScale));

    //            // 4) 부착: 움직임을 따라가도록 부모 컴포넌트에 부착
    //            CapComp->AttachToComponent(ParentComp, FAttachmentTransformRules::KeepWorldTransform);
    //        };

    //    // (A) 몸통 쪽 단면 덮기 (Stump) -> CutNormal 방향
    //    AddCapMesh(PMC_Stump, HitLocation, CutNormal);

    //    // (B) 잘린 팔 쪽 단면 덮기 (Debris) -> CutNormal 반대 방향
    //    AddCapMesh(PMC_Debris, HitLocation, -CutNormal);
    //}

    PMC_Stump->SetVisibility(true);
    PMC_Debris->SetVisibility(true);

    // 4. 물리 분리 (기존 코드 유지)
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetAllBodiesBelowSimulatePhysics(TargetBone, true); // TargetBone 이하는 물리 적용되어 떨어져 나감
    Mesh->BreakConstraint(-HitNormal * ImpulsePower, HitLocation, TargetBone);
    //Mesh->AddImpulse(-HitNormal * ImpulsePower, TargetBone, false);

    // 5. 마스킹
    USliceUtils::MaskTargetBoneOnly(Character->GetMesh(), TargetBone);

}

void USliceSystemComponent::SliceBone_DMC(FName TargetBone, const FVector& HitLocation, const FVector& HitNormal, const FVector& CutNormal, float ImpulsePower)
{
    
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;

    USkeletalMeshComponent* Mesh = Character->GetMesh();
    if (!Mesh || !DMC_Stump || !DMC_Debris || !PMC_Stump || !PMC_Debris) return;

    // -------------------------------------------------------------------------
    // 1. 현재 포즈 복제 (Snapshot)
    // -------------------------------------------------------------------------
    FGeometryScriptCopyMeshFromComponentOptions CopyOptions;
    CopyOptions.bWantNormals = true;
    CopyOptions.bWantTangents = true;

    // [중요] World Space로 복사해야 현재 애니메이션 변형이 적용된 월드 좌표를 얻습니다.
    // 나중에 컴포넌트에 붙일 때 Local로 변환하거나, 부모의 World Transform을 고려해야 합니다.
    FTransform MeshTransform;
    EGeometryScriptOutcomePins Outcome;

    // DMC_Stump에 일단 전체 메시를 복사 (이것을 원본으로 사용)
    DMC_Stump->GetDynamicMesh()->Reset();
    UGeometryScriptLibrary_SceneUtilityFunctions::CopyMeshFromComponent(
        Mesh,
        DMC_Stump->GetDynamicMesh(),
        CopyOptions,
        true, // bTransformToWorld: true 월드 좌표로
        MeshTransform,
        Outcome
    );

    if (Outcome == EGeometryScriptOutcomePins::Failure) return;

    // DMC_Debris에도 복사 (똑같은 원본 2개 생성)
    DMC_Debris->GetDynamicMesh()->Reset();
    UGeometryScriptLibrary_SceneUtilityFunctions::CopyMeshFromComponent(
        Mesh,
        DMC_Debris->GetDynamicMesh(),
        CopyOptions,
        true, // bTransformToWorld: true 월드 좌표로
        MeshTransform,
        Outcome
    );
    
    // 머터리얼 동기화
    int32 NumMaterials = Mesh->GetNumMaterials();
    for (int32 i = 0; i < NumMaterials; i++)
    {
        DMC_Stump->SetMaterial(i, Mesh->GetMaterial(i));
        DMC_Debris->SetMaterial(i, Mesh->GetMaterial(i));
    }

    // 월드 좌표계 기준 평면 생성
    FTransform CutPlaneTransform = FTransform(UKismetMathLibrary::MakeRotFromZ(CutNormal), HitLocation);

    FGeometryScriptMeshPlaneCutOptions PlaneCutOptions;
    PlaneCutOptions.bFillHoles = true;
    PlaneCutOptions.bFlipCutSide = true;
    PlaneCutOptions.HoleFillMaterialID = 10;

    UGeometryScriptLibrary_MeshBooleanFunctions::ApplyMeshPlaneCut(
        DMC_Stump->GetDynamicMesh(),
        CutPlaneTransform,
        PlaneCutOptions
    );

    PlaneCutOptions.bFlipCutSide = false;

    UGeometryScriptLibrary_MeshBooleanFunctions::ApplyMeshPlaneCut(
        DMC_Debris->GetDynamicMesh(),
        CutPlaneTransform,
        PlaneCutOptions
    );

    // 단면 머터리얼 적용 (옵션: 새로 생긴 폴리곤은 보통 마지막 Material ID를 가짐)
    // GeometryScript는 Cap에 대한 Material ID 할당 옵션이 제한적일 수 있으므로 
    // 필요 시 RemapMaterial 등으로 처리해야 함.

    // -------------------------------------------------------------------------
    // 3. DynamicMesh -> ProceduralMesh 변환 (물리 및 게임플레이용)
    // -------------------------------------------------------------------------

    // [Debris 처리]
    // Debris는 월드에 독립적으로 떨어져야 하므로 월드 좌표 그대로 PMC에 넣음
    USliceUtils::ConvertDynamicMeshToProcMesh(DMC_Debris, PMC_Debris);

    PMC_Debris->SetWorldTransform(FTransform::Identity);
    PMC_Debris->SetVisibility(true);
    PMC_Debris->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetBone);


    USliceUtils::ConvertDynamicMeshToProcMesh(DMC_Stump, PMC_Stump);

    PMC_Stump->SetVisibility(true);

    FName ParentBone = Mesh->GetParentBone(TargetBone);
    int32 TargetBoneIndex = Mesh->GetBoneIndex(TargetBone);

    if (TargetBoneIndex != INDEX_NONE && ParentBone != NAME_None)
    {
        PMC_Stump->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ParentBone);

        FTransform RefBoneTransform = Mesh->GetSkeletalMeshAsset()->GetRefSkeleton().GetRefBonePose()[TargetBoneIndex];
    }
    else
    {
        PMC_Stump->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetBone);
    }

    Mesh->SetVisibility(true); // 원본 숨김
    
    DMC_Stump->SetVisibility(false);
    DMC_Debris->SetVisibility(false);

    // 4. 물리 분리 (기존 코드 유지)
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetAllBodiesBelowSimulatePhysics(TargetBone, true); // TargetBone 이하는 물리 적용되어 떨어져 나감
    Mesh->BreakConstraint(-HitNormal * ImpulsePower, HitLocation, TargetBone);
    //Mesh->AddImpulse(-HitNormal * ImpulsePower, TargetBone, false);

    // 5. 마스킹
    USliceUtils::MaskTargetBoneOnly(Character->GetMesh(), TargetBone);
}

void USliceSystemComponent::CopyWeightAndSlice_DMC(FName TargetBone, const FVector& HitLocation, const FVector& HitNormal, const FVector& CutNormal, float ImpulsePower)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;

    USkeletalMeshComponent* Mesh = Character->GetMesh();
    if (!Mesh || !DMC_Stump || !DMC_Debris || !PMC_Stump || !PMC_Debris) return;

    FTransform MeshTransform;
    EGeometryScriptOutcomePins Outcome;

    FGeometryScriptCopyMeshFromAssetOptions CopyMeshFromAssetOptions;
 
    FGeometryScriptMeshReadLOD GeometryScriptMeshReadLOD;
    GeometryScriptMeshReadLOD.LODIndex = 0;
    // 이 함수가 스킨웨이트도 복사함
    /*
    * FSkeletalMeshLODRenderDataToDynamicMesh::Convert(
	const FSkeletalMeshLODRenderData* SkeletalMeshResources,
	const FReferenceSkeleton& RefSkeleton,
	const ConversionOptions& Options,
	FDynamicMesh3& OutputMesh) 
    */
    DMC_Stump->GetDynamicMesh()->Reset();
    UGeometryScriptLibrary_StaticMeshFunctions::CopyMeshFromSkeletalMesh(
        Mesh->GetSkeletalMeshAsset(),
        DMC_Stump->GetDynamicMesh(), 
        CopyMeshFromAssetOptions, 
        GeometryScriptMeshReadLOD,
        Outcome,
        nullptr
    );

    if (Outcome == EGeometryScriptOutcomePins::Failure)
    {
        NET_LOG(TEXT("!!!!!! CopyMeshFromSkeletalMesh Fail"));
        return;
    }

    DMC_Debris->GetDynamicMesh()->Reset();
    UGeometryScriptLibrary_StaticMeshFunctions::CopyMeshFromSkeletalMesh(
        Mesh->GetSkeletalMeshAsset(),
        DMC_Debris->GetDynamicMesh(),
        CopyMeshFromAssetOptions,
        GeometryScriptMeshReadLOD,
        Outcome,
        nullptr
    );

    if (Outcome == EGeometryScriptOutcomePins::Failure)
    {
        NET_LOG(TEXT("!!!!!! CopyMeshFromSkeletalMesh Fail"));
        return;
    }

    ApplySkinningWithDMCData(DMC_Stump, Mesh);
    // 월드 좌표계 기준 평면 생성
    //FTransform CutPlaneTransform = FTransform(UKismetMathLibrary::MakeRotFromZ(CutNormal), HitLocation);

    //FGeometryScriptMeshPlaneCutOptions PlaneCutOptions;
    //PlaneCutOptions.bFillHoles = true;
    //PlaneCutOptions.bFlipCutSide = true;
    //PlaneCutOptions.HoleFillMaterialID = 10;

    //UGeometryScriptLibrary_MeshBooleanFunctions::ApplyMeshPlaneCut(
    //    DMC_Stump->GetDynamicMesh(),
    //    CutPlaneTransform,
    //    PlaneCutOptions
    //);

    //PlaneCutOptions.bFlipCutSide = false;

    //UGeometryScriptLibrary_MeshBooleanFunctions::ApplyMeshPlaneCut(
    //    DMC_Debris->GetDynamicMesh(),
    //    CutPlaneTransform,
    //    PlaneCutOptions
    //);

    DMC_Stump->SetVisibility(true);
    DMC_Debris->SetVisibility(true);
}

void USliceSystemComponent::ApplySkinningWithDMCData(UDynamicMeshComponent* DMC, USkeletalMeshComponent* SkelMeshComp)
{
    // 1. 유효성 체크
    if (!DMC || !SkelMeshComp) return;

    USkeletalMesh* SkelAsset = SkelMeshComp->GetSkeletalMeshAsset();
    if (!SkelAsset) return;

    UDynamicMesh* DynMesh = DMC->GetDynamicMesh();
    if (!DynMesh) return;

    NET_LOG(TEXT(""));

    // 2. 데이터 준비
    // A. 현재 프레임의 뼈 변환 (Component Space)
    const TArray<FTransform>& CurrentBoneTransforms = SkelMeshComp->GetComponentSpaceTransforms();

    // B. T-Pose 역행렬 (Ref-Pose Inverse)
    // UE5.0+ 에서는 GetRefBasesInvMatrix()가 public 접근 가능합니다.
    const TArray<FMatrix44f>& RefInvMatrices = SkelAsset->GetRefBasesInvMatrix();

    // 데이터 개수 불일치 방어
    int32 NumBones = RefInvMatrices.Num();
    if (CurrentBoneTransforms.Num() < NumBones) return;

    // 3. [최적화] 스키닝 행렬 미리 계산 (Pre-calculate Skinning Matrices)
    // 루프 안에서 매번 행렬 곱을 하지 않고, 뼈 개수만큼만 미리 계산합니다.
    TArray<FMatrix> SkinningMatrices;
    SkinningMatrices.SetNumUninitialized(NumBones);

    for (int32 i = 0; i < NumBones; i++)
    {
        // RefInv(Local->Bone) * Current(Bone->Component)
        // FMatrix44f를 FMatrix(double 정밀도)로 변환
        FMatrix RefInvMat = FMatrix(RefInvMatrices[i]);
        FMatrix CurrentMat = CurrentBoneTransforms[i].ToMatrixWithScale();

        // 언리얼 행렬 곱 순서: V_New = V_Old * (RefInv * Current)
        SkinningMatrices[i] = RefInvMat * CurrentMat;
    }

    // DMC가 SkelMesh와 다른 Transform을 가질 경우를 대비해 Local 변환 행렬 준비
    // 만약 DMC가 SkelMesh 자식으로 0,0,0에 붙어있다면 Identity이므로 영향 없음
    FTransform DMCToWorld = DMC->GetComponentTransform();
    FTransform SkelToWorld = SkelMeshComp->GetComponentTransform();
    // SkelMesh Component Space -> World -> DMC Local Space
    // (보통 Slice 시에는 DMC를 World에 고정하므로 이 변환이 중요할 수 있습니다)
    FMatrix ComponentToDMCLocal = (SkelToWorld * DMCToWorld.Inverse()).ToMatrixWithScale();


    // 4. 메쉬 편집 (CPU Skinning)
    DynMesh->EditMesh([&](UE::Geometry::FDynamicMesh3& Mesh)
        {
            // SkinWeights Attribute 확인
            if (!Mesh.HasAttributes() || !Mesh.Attributes()->HasSkinWeightsAttribute(FName("SkinWeights")))
            {
                // 웨이트 정보가 없으면 스키닝 불가
                return;
            }

            const auto* SkinWeightsAttr = Mesh.Attributes()->GetSkinWeightsAttribute(FName("SkinWeights"));

            TArray<int32> BoneIndices;
            TArray<float> BoneWeights;

            // 버텍스 루프
            for (int32 VertID : Mesh.VertexIndicesItr())
            {
                // *주의*: 여기서 GetVertex는 반드시 'T-Pose(Bind Pose)' 상태의 위치여야 합니다.
                // 이미 변형된 메쉬라면 이 함수를 두 번 실행했을 때 모양이 이상해집니다.
                FVector OriginalPos = Mesh.GetVertex(VertID);

                FVector FinalPosCompSpace = FVector::ZeroVector; // Component Space 위치

                
                //GetValue에 빈 배열을 넘겨주면, 함수가 알아서 채움
                SkinWeightsAttr->GetValue(VertID, BoneIndices, BoneWeights);

                float TotalWeight = 0.0f;

                for (int32 i = 0; i < BoneWeights.Num(); ++i)
                {
                    int32 BoneIndex = BoneIndices[i];
                    float Weight = BoneWeights[i];

                    if (Weight < KINDA_SMALL_NUMBER) continue;

                    if (SkinningMatrices.IsValidIndex(BoneIndex))
                    {
                        // 미리 계산된 행렬 사용
                        FinalPosCompSpace += SkinningMatrices[BoneIndex].TransformPosition(OriginalPos) * Weight;
                        TotalWeight += Weight;
                    }
                }

                // 웨이트가 유효하다면 위치 업데이트
                if (TotalWeight > 0.0f)
                {
                    // Component Space -> DMC Local Space 변환 후 적용
                    // (DMC와 SkelMesh 위치가 같다면 FinalPosCompSpace 그대로 사용 가능)
                    FVector FinalPosLocal = ComponentToDMCLocal.TransformPosition(FinalPosCompSpace);

                    Mesh.SetVertex(VertID, FinalPosLocal);
                }
            }
        }, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::VertexPositions, false);

    // 5. 노멀 및 물리 업데이트
    // 위치 변경 후 노멀 재계산은 필수 (안 하면 쉐이딩 깨짐)
    // (UE::Geometry::FDynamicMesh3 내부 함수 혹은 라이브러리 사용)
    //UE::Geometry::FMeshNormals::QuickComputeVertexNormals(*DynMesh->GetMeshPtr());

    // 변경 사항 알림
    DMC->NotifyMeshUpdated();

    // 필요 시 콜리전 업데이트 (비용이 크므로 상황에 따라 호출)
    // DMC->UpdateCollisionFromMesh(); 
}

void USliceSystemComponent::SetupPMCs()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    // 런타임에 PMC 생성 및 등록
    PMC_Stump = NewObject<UProceduralMeshComponent>(Owner, TEXT("PMC_Stump"));
    PMC_Stump->RegisterComponent();
    PMC_Stump->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    PMC_Stump->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PMC_Stump->SetVisibility(false);

    PMC_Debris = NewObject<UProceduralMeshComponent>(Owner, TEXT("PMC_Debris"));
    PMC_Debris->RegisterComponent();
    PMC_Debris->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    PMC_Debris->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    PMC_Debris->SetVisibility(false);
}

void USliceSystemComponent::SetupDMCs()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    // Stump 생성 (몸통 부착용)
    DMC_Stump = NewObject<UDynamicMeshComponent>(Owner, TEXT("DMC_Stump"));
    DMC_Stump->RegisterComponent();
    DMC_Stump->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Stump는 보통 충돌 끔
    DMC_Stump->SetVisibility(false);

    // Debris 생성 (물리용)
    DMC_Debris = NewObject<UDynamicMeshComponent>(Owner, TEXT("DMC_Debris"));
    DMC_Debris->RegisterComponent();
    DMC_Debris->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    DMC_Debris->EnableComplexAsSimpleCollision(); // 복잡한 모양대로 물리 적용
    DMC_Debris->SetVisibility(false);
}

float USliceSystemComponent::GetBoneRadius(USkeletalMeshComponent* Mesh, FName BoneName)
{
    if (!Mesh || !Mesh->GetPhysicsAsset()) return 10.0f; // 기본값

    // 해당 본에 연결된 바디 셋업 인덱스 찾기
    int32 BodyIndex = Mesh->GetPhysicsAsset()->FindBodyIndex(BoneName);

    if (BodyIndex != INDEX_NONE)
    {
        // 바디 셋업(BodySetup) 가져오기
        USkeletalBodySetup* BodySetup = Mesh->GetPhysicsAsset()->SkeletalBodySetups[BodyIndex];
        // 캡슐(Sphyl) 정보가 있는지 확인
        if (BodySetup->AggGeom.SphylElems.Num() > 0)
        {
            // 캡슐의 반지름 = 뼈의 굵기
            return BodySetup->AggGeom.SphylElems[0].Radius;
        }
    }

    return 10.0f; // 실패 시 기본값
}