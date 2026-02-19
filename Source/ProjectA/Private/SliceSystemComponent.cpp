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

//#include "SkeletalMeshLODRenderDataToDynamicMesh.h" 

//#include <GeometryScriptingCore/Private/MeshAssetFunctions.cpp>
//#include <GeometryScriptingCore/Public/GeometryScript/MeshAssetFunctions.h>

// Sets default values for this component's properties
USliceSystemComponent::USliceSystemComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

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

    //NET_LOG(TEXT(""));
    PrecomputeSkinningMatrices();
    UpdatePMCSkinning(PMC_Stump);
    UpdatePMCSkinning(PMC_Debris);
}

void USliceSystemComponent::SliceBone(FName TargetBone, const FVector& HitLocation, const FVector& HitNormal, const FVector& CutNormal, float ImpulsePower)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;

    USkeletalMeshComponent* Mesh = Character->GetMesh();

    // 1. 유틸 함수 호출 (이제 PMC_Stump는 TargetBone의 로컬 좌표계로 데이터가 채워짐)
    USliceUtils::ConvertBoneToProcMesh(Mesh, TargetBone, PMC_Stump.ProcMeshComp);

    // [중요] 자르기 전에 'Stump'를 타겟 본에 먼저 붙여야, Slice 함수가 월드 좌표를 로컬로 올바르게 변환함
    // 또한 애니메이션 위치에 맞게 메시가 정렬됨
    PMC_Stump.ProcMeshComp->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetBone);
    PMC_Stump.ProcMeshComp->SetRelativeTransform(FTransform::Identity); // 로컬 좌표계이므로 0,0,0으로 초기화
    PMC_Stump.ProcMeshComp->UpdateComponentToWorld(); // 월드 좌표 갱신 보장

    // 2. 자르기
    // PMC_Stump가 이미 본 위치에 가 있으므로, Slice 함수가 내부적으로 HitLocation을 로컬로 잘 변환함
    FVector CutLocation = PMC_Stump.ProcMeshComp->Bounds.Origin;

	UMaterialInterface* CapMaterial = SliceCapMaterial ? SliceCapMaterial : Mesh->GetMaterial(0);
    UKismetProceduralMeshLibrary::SliceProceduralMesh(
        PMC_Stump.ProcMeshComp, HitLocation, FVector::UpVector, true,
        PMC_Debris.ProcMeshComp, EProcMeshSliceCapOption::CreateNewSectionForCap,
        CapMaterial
    );

    //// 3. 위치 재조정 및 부착

    // [Debris 설정] : 잘려나간 부위
    // Debris는 TargetBone(물리 시뮬레이션 될 본)에 붙입니다.
    PMC_Debris.ProcMeshComp->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetBone);
    PMC_Debris.ProcMeshComp->SetRelativeTransform(FTransform::Identity); // 오프셋 없이 본 위치에 일치시킴
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
        PMC_Stump.ProcMeshComp->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ParentBone);

        // 2. 오프셋 적용 (TargetBone의 로컬 Transform을 적용)
        FTransform RefBoneTransform = Mesh->GetSkeletalMeshAsset()->GetRefSkeleton().GetRefBonePose()[TargetBoneIndex];
        PMC_Stump.ProcMeshComp->SetRelativeTransform(RefBoneTransform);
    }
    else
    {
        // 부모가 없는 루트 본이거나 오류 상황이면 그냥 둠
        PMC_Stump.ProcMeshComp->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetBone);
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

    PMC_Stump.ProcMeshComp->SetVisibility(true);
    PMC_Debris.ProcMeshComp->SetVisibility(true);

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
    if (!Mesh || !DMC_Stump || !DMC_Debris) return;

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
    USliceUtils::ConvertDynamicMeshToProcMesh(DMC_Debris, PMC_Debris.ProcMeshComp);
    USliceUtils::ConvertDynamicMeshToProcMesh(DMC_Stump, PMC_Stump.ProcMeshComp);

    //PMC_Debris.ProcMeshComp->SetWorldTransform(FTransform::Identity);
    //PMC_Debris.ProcMeshComp->SetVisibility(true);
    //PMC_Debris.ProcMeshComp->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetBone);
    //PMC_Stump->SetVisibility(true);

    

    

    FName ParentBone = Mesh->GetParentBone(TargetBone);
    int32 TargetBoneIndex = Mesh->GetBoneIndex(TargetBone);

    if (TargetBoneIndex != INDEX_NONE && ParentBone != NAME_None)
    {
        DMC_Stump->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ParentBone);

        FTransform RefBoneTransform = Mesh->GetSkeletalMeshAsset()->GetRefSkeleton().GetRefBonePose()[TargetBoneIndex];
    }
    else
    {
        DMC_Stump->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetBone);
    }

    Mesh->SetVisibility(true); // 원본 숨김
    
    DMC_Stump->SetVisibility(true);
    DMC_Debris->SetVisibility(true);

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
    /*
    * 스켈레탈 메쉬 Allow CPU Access true !!
    */
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;

    USkeletalMeshComponent* Mesh = Character->GetMesh();
    if (!Mesh || !DMC_Stump || !DMC_Debris) return;

    FTransform MeshTransform;
    EGeometryScriptOutcomePins Outcome;

    FGeometryScriptCopyMeshFromAssetOptions CopyMeshFromAssetOptions;
    CopyMeshFromAssetOptions.bRequestTangents = true;

    FGeometryScriptMeshReadLOD GeometryScriptMeshReadLOD;
    GeometryScriptMeshReadLOD.LODIndex = 0;
    GeometryScriptMeshReadLOD.LODType = EGeometryScriptLODType::MaxAvailable;
    // 이 함수가 스킨웨이트도 복사함
    /*
    * FSkeletalMeshLODRenderDataToDynamicMesh::Convert(
	const FSkeletalMeshLODRenderData* SkeletalMeshResources,
	const FReferenceSkeleton& RefSkeleton,
	const ConversionOptions& Options,
	FDynamicMesh3& OutputMesh) 
    */

    //UELocal::CopyMeshFromSkeletalMesh_RenderData()
    //UE::Geometry::FSkelalmesh
    /*	const FSkeletalMeshLODRenderData* SkeletalMeshResources,
	const FReferenceSkeleton& RefSkeleton,
	const ConversionOptions& Options,
	FDynamicMesh3& OutputMesh,
	bool bHasVertexColors,
	TFunctionRef<FColor(int32)> GetVertexColorFromLODVertexIndex)
    */
 /*   UELocal::CopyMeshFromSkeletalMesh_RenderData(
        Mesh->GetSkeletalMeshAsset(), 
        CopyMeshFromAssetOptions, 
        GeometryScriptMeshReadLOD.LODIndex, 
        DMC_Stump->GetDynamicMesh(), 
        nullptr);*/
    DMC_Stump->GetDynamicMesh()->Reset();
    //USliceUtils::InitializeDMCFromSkeletalMesh(DMC_Stump, Mesh, Outcome);
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
        NET_LOG(TEXT("Error: InitializeDMCFromSkeletalMesh fail"));
        return;
    }

    DMC_Debris->GetDynamicMesh()->Reset();
    //USliceUtils::InitializeDMCFromSkeletalMesh(DMC_Debris, Mesh, Outcome);
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
        NET_LOG(TEXT("Error: InitializeDMCFromSkeletalMesh fail"));
        return;
    }

    // 머터리얼 에셋 동기화
    int32 NumMaterials = Mesh->GetNumMaterials();
    for (int32 i = 0; i < NumMaterials; i++)
    {
        UMaterialInterface* Mat = Mesh->GetMaterial(i);

        // DMC에 머터리얼 할당
        DMC_Stump->SetMaterial(i, Mat);
        DMC_Debris->SetMaterial(i, Mat);
    }

    // 1. 월드 공간의 절단 평면 Transform 생성
    FTransform CutPlaneWorldTransform = FTransform(UKismetMathLibrary::MakeRotFromZ(CutNormal), HitLocation);

    // 2. 기준이 될 뼈(TargetBone)의 현재 월드 Transform 가져오기 (애니메이션 적용됨)
    FTransform BoneWorldTransform = Mesh->GetSocketTransform(TargetBone, RTS_World);

    // 3. 절단면이 뼈 기준으로 어디에 있는지(상대 좌표) 계산
    // "뼈가 이만큼 움직일 때 절단면도 같이 움직였다"고 가정
    FTransform PlaneRelativeToBone = CutPlaneWorldTransform.GetRelativeTransform(BoneWorldTransform);

    // 4. T-Pose 상태(Ref Pose)에서의 뼈 Transform 계산
    // (DMC는 T-Pose 상태이므로, T-Pose 기준의 뼈 위치를 알아야 함)
    const FReferenceSkeleton& RefSkeleton = Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();
    int32 BoneIndex = Mesh->GetBoneIndex(TargetBone);

    TArray<FTransform> ComponentSpaceTransforms;
    FTransform BoneRefPoseCompSpace = FTransform::Identity;
    if (BoneIndex != INDEX_NONE)
    {
        // RefPose의 Component Space Transform을 구하는 함수
        // (단일 본만 구하면 비효율적일 수 있으나 로직 설명상 명확함. 
        //  최적화하려면 FillUpComponentSpaceTransforms를 사용해 전체를 캐싱하는 것이 좋음)
        FAnimationRuntime::FillUpComponentSpaceTransforms(RefSkeleton, RefSkeleton.GetRefBonePose(), ComponentSpaceTransforms);

        if (ComponentSpaceTransforms.IsValidIndex(BoneIndex))
        {
            BoneRefPoseCompSpace = ComponentSpaceTransforms[BoneIndex];
        }
    }

    // 5. 최종: T-Pose 뼈 위치에 아까 구한 상대 좌표를 적용 -> DMC 기준 절단면 완성
    FTransform CutPlaneInDMCRefPoseSpace = PlaneRelativeToBone * BoneRefPoseCompSpace;

	TSet<int32> TargetBoneIndices;
    //TargetBoneIndices.Add(BoneIndex);
	USliceUtils::FindPlaneCutBones(Mesh->GetSkeletalMeshAsset(), CutPlaneInDMCRefPoseSpace, TargetBoneIndices);
    if (!CutableBones.IsEmpty())
    {
        //교집합
        for (auto It = TargetBoneIndices.CreateIterator(); It; ++It)
        {
            int32 CurIdx = *It;

            if (!CutableBones.Contains(Mesh->GetBoneName(CurIdx)))
            {

                It.RemoveCurrent();
            }
        }
    }

    //고정으로 테스트
    FName FixedBoneName = FName("Spine1");
    int32 FixedBoneIndex = Mesh->GetBoneIndex(FixedBoneName);

    if (FixedBoneIndex != INDEX_NONE && ComponentSpaceTransforms.IsValidIndex(FixedBoneIndex))
    {
        FVector FixedBoneLocation = ComponentSpaceTransforms[FixedBoneIndex].GetLocation();
        CutPlaneInDMCRefPoseSpace.SetLocation(FixedBoneLocation);
        TargetBoneIndices.Empty();
        TargetBoneIndices.Add(FixedBoneIndex);
    }

    FGeometryScriptMeshPlaneCutOptions PlaneCutOptions;
    PlaneCutOptions.bFillHoles = true;
    PlaneCutOptions.bFlipCutSide = true;
	PlaneCutOptions.HoleFillMaterialID = 10;

    UGeometryScriptLibrary_MeshBooleanFunctions::ApplyMeshPlaneCut(
        DMC_Stump->GetDynamicMesh(),
        CutPlaneInDMCRefPoseSpace,
        PlaneCutOptions
    );

    PlaneCutOptions.bFlipCutSide = false;

    UGeometryScriptLibrary_MeshBooleanFunctions::ApplyMeshPlaneCut(
        DMC_Debris->GetDynamicMesh(),
        CutPlaneInDMCRefPoseSpace,
        PlaneCutOptions
    );

    if (SliceCapMaterial)
    {
        DMC_Stump->SetMaterial(10, SliceCapMaterial);
        DMC_Debris->SetMaterial(10, SliceCapMaterial);
    }

    DMC_Stump->SetVisibility(true);
    DMC_Debris->SetVisibility(true);
    DMC_Debris->AddLocalOffset(FVector(0, 0, 20.f));

    USliceUtils::ConvertDynamicMeshToProcMesh(DMC_Stump, PMC_Stump.ProcMeshComp, PMC_Stump.SkinCache, TargetBoneIndices);
    USliceUtils::ConvertDynamicMeshToProcMesh(DMC_Debris, PMC_Debris.ProcMeshComp, PMC_Debris.SkinCache, TargetBoneIndices);

    PMC_Stump.ProcMeshComp->SetVisibility(true);
    PMC_Debris.ProcMeshComp->SetVisibility(true);
	PMC_Stump.ProcMeshComp->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    PMC_Debris.ProcMeshComp->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    //PMC_Debris.ProcMeshComp->AddLocalOffset(FVector(0, 0, 20.f));


    // Stump는 TargetBone(잘린 뼈)과 그 자식들의 웨이트를 버림
    RefineSkinWeights(PMC_Stump, TargetBoneIndices, true);

    // Debris는 TargetBone과 그 자식들의 웨이트만 가짐 (나머지 버림)
    RefineSkinWeights(PMC_Debris, TargetBoneIndices, false);

    InitializePMCBuffers(PMC_Stump);
    InitializePMCBuffers(PMC_Debris);

    PMC_Stump.bUpdateSkinning = true;
    PMC_Debris.bUpdateSkinning = true;

	//Mesh->SetVisibility(false); // 원본 숨김
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    //Mesh->AddImpulse(-HitNormal * ImpulsePower, TargetBone, false);

    // 5. 마스킹
    for (auto BoneIdx : TargetBoneIndices)
    {
		FName BoneName = Mesh->GetBoneName(BoneIdx);
        Mesh->SetAllBodiesBelowSimulatePhysics(BoneName, true); // TargetBone 이하는 물리 적용되어 떨어져 나감
        Mesh->BreakConstraint(-HitNormal * ImpulsePower, HitLocation, BoneName);
        USliceUtils::MaskTargetBoneOnly(Character->GetMesh(), BoneName);
    }
}

void USliceSystemComponent::SetupPMCs()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    // 런타임에 PMC 생성 및 등록
    PMC_Stump.ProcMeshComp = NewObject<UProceduralMeshComponent>(Owner, TEXT("PMC_Stump"));
    PMC_Stump.ProcMeshComp->RegisterComponent();
    PMC_Stump.ProcMeshComp->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    PMC_Stump.ProcMeshComp->SetCollisionProfileName(FName("PMC"), true);
    PMC_Stump.ProcMeshComp->SetVisibility(false);
    //PMC_Stump->UpdateMeshSection_LinearColor()

    PMC_Debris.ProcMeshComp = NewObject<UProceduralMeshComponent>(Owner, TEXT("PMC_Debris"));
    PMC_Debris.ProcMeshComp->RegisterComponent();
    PMC_Debris.ProcMeshComp->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    PMC_Debris.ProcMeshComp->SetCollisionProfileName(FName("PMC"), true);
    PMC_Debris.ProcMeshComp->SetVisibility(false);
}

void USliceSystemComponent::SetupDMCs()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    // Stump 생성
    DMC_Stump = NewObject<UDynamicMeshComponent>(Owner, TEXT("DMC_Stump"));
    DMC_Stump->RegisterComponent();
    DMC_Stump->SetCollisionProfileName(FName("PMC"), true); // Stump는 보통 충돌 끔
    DMC_Stump->SetVisibility(false);

    // Debris 생성
    DMC_Debris = NewObject<UDynamicMeshComponent>(Owner, TEXT("DMC_Debris"));
    DMC_Debris->RegisterComponent();
    DMC_Debris->SetCollisionProfileName(FName("PMC"), true); // Stump는 보통 충돌 끔
    //DMC_Debris->EnableComplexAsSimpleCollision(); // 복잡한 모양대로 물리 적용
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

void USliceSystemComponent::RefineSkinWeights(FSlicePMC& InSlicePMC, const TSet<int32>& CutBoneIndices, bool bIsStump)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character || !Character->GetMesh()) return;

    const FReferenceSkeleton& RefSkeleton = Character->GetMesh()->GetSkeletalMeshAsset()->GetRefSkeleton();
    int32 NumBones = RefSkeleton.GetNum();

    if (CutBoneIndices.Num() == 0) return;

    int32 PrimaryCutBone = -1;
    for (int32 idx : CutBoneIndices) { PrimaryCutBone = idx; break; }

    // 1. Debris(잘려나간 부위) 마스크 생성
    TArray<bool> IsDebrisBone;
    IsDebrisBone.Init(false, NumBones);
    for (int32 i = 1; i < NumBones; ++i)
    {
        // 자신이 잘린 뼈이거나, 부모가 잘린 뼈라면 Debris로 판정 (계층구조 순회 보장)
        if (CutBoneIndices.Contains(i) || (RefSkeleton.GetParentIndex(i) != INDEX_NONE && IsDebrisBone[RefSkeleton.GetParentIndex(i)]))
        {
            IsDebrisBone[i] = true;
        }
    }

    // 2. 웨이트 이전 맵(Bone Remap) 구성 ★핵심★
    TArray<int32> BoneMap;
    BoneMap.Init(0, NumBones);

    if (bIsStump)
    {
        // [Stump 로직]: 잘려나간 뼈들의 웨이트를 그 부모 뼈(살아남은 뼈)에게 넘겨줌
        for (int32 i = 0; i < NumBones; ++i) BoneMap[i] = i; // 기본은 자기 자신

        for (int32 i = 1; i < NumBones; ++i)
        {
            if (CutBoneIndices.Contains(i))
            {
                int32 ParentIdx = RefSkeleton.GetParentIndex(i);
                BoneMap[i] = (ParentIdx != INDEX_NONE) ? ParentIdx : 0;
            }
            else
            {
                int32 ParentIdx = RefSkeleton.GetParentIndex(i);
                if (ParentIdx != INDEX_NONE && BoneMap[ParentIdx] != ParentIdx)
                {
                    // 부모가 매핑되었다면 나도 부모를 따라감
                    BoneMap[i] = BoneMap[ParentIdx];
                }
            }
        }
    }
    else
    {
        // [Debris 로직]: 몸통에 남은 뼈들의 웨이트를 잘려나간 가장 최상위 뼈에게 넘겨줌
        for (int32 i = 0; i < NumBones; ++i)
        {
            BoneMap[i] = IsDebrisBone[i] ? i : PrimaryCutBone;
        }
    }

    // 3. 버텍스 웨이트 재분배 (정규화 불필요, 총합은 알아서 1.0 유지됨)
    for (FCachedSkinVertex& V : InSlicePMC.SkinCache)
    {
        int32 TempIndices[4] = { 0, 0, 0, 0 };
        float TempWeights[4] = { 0.f, 0.f, 0.f, 0.f };
        int32 UniqueCount = 0;

        for (int32 w = 0; w < 4; ++w)
        {
            if (V.BoneWeights[w] <= 0.001f) continue;

            // 웨이트를 이전받을 뼈 확인
            int32 MappedBone = BoneMap[V.BoneIndices[w]];

            // 이미 해당 뼈가 배열에 있다면 웨이트 합치기
            bool bFound = false;
            for (int32 k = 0; k < UniqueCount; ++k)
            {
                if (TempIndices[k] == MappedBone)
                {
                    TempWeights[k] += V.BoneWeights[w];
                    bFound = true;
                    break;
                }
            }

            // 배열에 없다면 새로 추가
            if (!bFound && UniqueCount < 4)
            {
                TempIndices[UniqueCount] = MappedBone;
                TempWeights[UniqueCount] = V.BoneWeights[w];
                UniqueCount++;
            }
        }

        // 결과 덮어쓰기
        FMemory::Memzero(V.BoneIndices, sizeof(V.BoneIndices));
        FMemory::Memzero(V.BoneWeights, sizeof(V.BoneWeights));
        for (int32 k = 0; k < UniqueCount; ++k)
        {
            V.BoneIndices[k] = TempIndices[k];
            V.BoneWeights[k] = TempWeights[k];
        }
    }
}

//void USliceSystemComponent::RefineSkinWeights(FSlicePMC& InSlicePMC, const TSet<int32>& CutBoneIndices, bool bIsStump)
//{
//    ACharacter* Character = Cast<ACharacter>(GetOwner());
//    if (!Character || !Character->GetMesh()) return;
//
//    const FReferenceSkeleton& RefSkeleton = Character->GetMesh()->GetSkeletalMeshAsset()->GetRefSkeleton();
//    int32 NumBones = RefSkeleton.GetNum();
//
//    if (CutBoneIndices.Num() == 0) return;
//
//    // ---------------------------------------------------------
//    // 1. "Debris에 속하는 본" 전체 목록 작성 (Pre-calculation)
//    // ---------------------------------------------------------
//    TArray<bool> IsDebrisBone;
//    IsDebrisBone.Init(false, NumBones);
//
//    // (1) 절단면 본들(Root of Debris)을 먼저 마킹
//    for (int32 CutBoneIdx : CutBoneIndices)
//    {
//        if (IsDebrisBone.IsValidIndex(CutBoneIdx))
//        {
//            IsDebrisBone[CutBoneIdx] = true;
//        }
//    }
//
//    // (2) 계층 구조를 순회하며 Debris의 자식들도 모두 Debris로 마킹
//    // * 중요: 본 인덱스는 부모 < 자식 순서가 보장되므로 1회 순회로 충분함
//    for (int32 i = 1; i < NumBones; ++i)
//    {
//        int32 ParentIndex = RefSkeleton.GetParentIndex(i);
//        if (ParentIndex != INDEX_NONE && IsDebrisBone[ParentIndex])
//        {
//            IsDebrisBone[i] = true;
//        }
//    }
//
//    // ---------------------------------------------------------
//    // 2. 고아 버텍스 처리를 위한 Fallback 본 인덱스 결정
//    // ---------------------------------------------------------
//    // 여러 개의 뼈가 잘렸을 때, 대표로 사용할 뼈 하나를 정합니다. (보통 첫 번째 것)
//    int32 PrimaryCutBone = -1;
//    for (int32 idx : CutBoneIndices) { PrimaryCutBone = idx; break; }
//
//    int32 FallbackBoneIndex = 0; // Default Root
//
//    if (bIsStump)
//    {
//        // Stump인데 웨이트가 다 사라졌다 -> 잘린 뼈의 부모(몸통 쪽)에 붙임
//        int32 ParentIdx = RefSkeleton.GetParentIndex(PrimaryCutBone);
//        FallbackBoneIndex = (ParentIdx != INDEX_NONE) ? ParentIdx : 0;
//    }
//    else
//    {
//        // Debris인데 웨이트가 다 사라졌다 -> 잘린 뼈(파편 쪽)에 붙임
//        FallbackBoneIndex = PrimaryCutBone;
//    }
//
//
//    // ---------------------------------------------------------
//    // 3. 버텍스 웨이트 정제 (Single Pass)
//    // ---------------------------------------------------------
//    for (FCachedSkinVertex& V : InSlicePMC.SkinCache)
//    {
//        //// [핵심 추가] 단면(Cap) 버텍스라면 무조건 단일 뼈에 100% 웨이트 할당 (계단 현상 방지)
//        //if (V.SectionIndex == 10)
//        //{
//        //    FMemory::Memzero(V.BoneWeights, sizeof(V.BoneWeights)); // 웨이트 초기화
//        //    V.BoneIndices[0] = FallbackBoneIndex; // Stump면 부모 뼈, Debris면 잘린 뼈
//        //    V.BoneWeights[0] = 1.0f;              // 100% 강제 할당
//        //    continue; // 보간 웨이트 필터링 로직 건너뜀
//        //}
//
//        float NewWeights[4] = { 0.f, 0.f, 0.f, 0.f };
//        float TotalWeight = 0.0f;
//
//        for (int32 w = 0; w < 4; ++w)
//        {
//            int32 BoneIdx = V.BoneIndices[w];
//            float OrigWeight = V.BoneWeights[w];
//
//            if (OrigWeight <= 0.001f) continue;
//
//            bool bBoneIsDebris = IsDebrisBone.IsValidIndex(BoneIdx) ? IsDebrisBone[BoneIdx] : false;
//
//            // [로직] 필터링
//            if (bIsStump && bBoneIsDebris)
//            {
//                // Stump(몸통)인데 Debris(잘린 뼈) 웨이트를 가짐 -> 제거 대상
//                NewWeights[w] = 0.0f;
//            }
//            else if (!bIsStump && !bBoneIsDebris)
//            {
//                // Debris(파편)인데 Stump(몸통) 웨이트를 가짐 -> 제거 대상
//                NewWeights[w] = 0.0f;
//            }
//            else
//            {
//                // 유지
//                NewWeights[w] = OrigWeight;
//            }
//
//            TotalWeight += NewWeights[w];
//        }
//
//        if (TotalWeight > 0.001f)
//        {
//            // [정상 케이스] 남은 웨이트가 있으면 정규화해서 적용
//            float Scale = 1.0f / TotalWeight;
//            for (int32 w = 0; w < 4; ++w)
//            {
//                V.BoneWeights[w] = NewWeights[w] * Scale;
//            }
//        }
//        else
//        {
//            // [문제 해결] 웨이트가 다 사라진 고아 버텍스 발생! (표면 경계면)
//
//            // Stump(몸통)라면 -> 잘린 뼈의 부모(어깨/몸통)에 강제로 붙임
//            // Debris(파편)라면 -> 잘린 뼈(상박)에 강제로 붙임
//
//            // PrimaryCutBone: 아까 밖에서 구해둔 잘린 뼈 인덱스
//            // FallbackBoneIndex: Stump면 부모본, Debris면 자기자신 (함수 도입부에서 계산 필요)
//
//            FMemory::Memzero(V.BoneWeights, sizeof(V.BoneWeights)); // 0으로 초기화
//            V.BoneIndices[0] = FallbackBoneIndex; // 강제 할당
//            V.BoneWeights[0] = 1.0f;
//        }
//    }
//}

void USliceSystemComponent::InitializePMCBuffers(FSlicePMC& InSlicePMC)
{
    if (!InSlicePMC.ProcMeshComp) return;

    InSlicePMC.UpdateBuffers.Empty();

    int32 NumSections = InSlicePMC.ProcMeshComp->GetNumSections();
    for (int32 SecIdx = 0; SecIdx < NumSections; SecIdx++)
    {
        FProcMeshSection* Section = InSlicePMC.ProcMeshComp->GetProcMeshSection(SecIdx);
        if (Section && Section->ProcVertexBuffer.Num() > 0)
        {
            FProcMeshSectionBuffer& Buffer = InSlicePMC.UpdateBuffers.Add(SecIdx);

            int32 NumVerts = Section->ProcVertexBuffer.Num();

            //메모리 할당
            Buffer.Vertices.SetNumUninitialized(NumVerts);
            Buffer.Normals.SetNumUninitialized(NumVerts);
            Buffer.UVs.SetNumUninitialized(NumVerts);
            Buffer.Colors.SetNumUninitialized(NumVerts);
            Buffer.Tangents.SetNumUninitialized(NumVerts);
            Buffer.InitialTangents.SetNumUninitialized(NumVerts);

            //기존 데이터 백업
            // FProcMeshVertex 구조체에서 데이터를 분리하여 Buffer 배열에 옮겨담아야 함
            for (int32 i = 0; i < NumVerts; ++i)
            {
                const FProcMeshVertex& SrcVert = Section->ProcVertexBuffer[i];

                // 위치와 노멀은 어차피 매 프레임 덮어쓰지만 초기값으로 넣어둠
                Buffer.Vertices[i] = SrcVert.Position;
                Buffer.Normals[i] = SrcVert.Normal;

                // [핵심] 정적 데이터 백업 (이게 없으면 텍스처 안 나옴)
                Buffer.UVs[i] = SrcVert.UV0;
                Buffer.Colors[i] = FLinearColor(SrcVert.Color); // FColor -> FLinearColor 변환
                //Buffer.Colors[i] = FLinearColor::White;
                Buffer.Tangents[i] = SrcVert.Tangent;

                // [추가] 초기 탄젠트 값 저장 (T-Pose 기준)
                Buffer.InitialTangents[i] = SrcVert.Tangent;
            }
        }
    }
}

void USliceSystemComponent::PrecomputeSkinningMatrices()
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;
    USkeletalMeshComponent* Mesh = Character->GetMesh();
    if (!Mesh) return;

    // (A) 현재 본 트랜스폼 (Component Space)
    const TArray<FTransform>& ComponentSpaceTransforms = Mesh->GetComponentSpaceTransforms();

    // (B) 레퍼런스 포즈 역행렬
    const auto& RefInvMatrices = Mesh->GetSkeletalMeshAsset()->GetRefBasesInvMatrix();

    int32 NumBones = RefInvMatrices.Num();
    if (SkinningMatrices.Num() != NumBones)
    {
        SkinningMatrices.SetNumUninitialized(NumBones);
    }

    // (C) 행렬 곱 계산 (RefInv * Current)
    ParallelFor(NumBones, [&](int32 BoneIdx)
        {
            if (ComponentSpaceTransforms.IsValidIndex(BoneIdx))
            {
                FMatrix RefInv = (FMatrix)RefInvMatrices[BoneIdx];
                FMatrix Current = ComponentSpaceTransforms[BoneIdx].ToMatrixWithScale();
                SkinningMatrices[BoneIdx] = RefInv * Current;
            }
        });
}

void USliceSystemComponent::UpdatePMCSkinning(FSlicePMC& InSlicePMC)
{
    // 1. 유효성 검사 (Fail Fast)
    // PMC가 없거나, 캐시 데이터가 없거나, 본 행렬이 준비되지 않았으면 리턴
    if (!InSlicePMC.bUpdateSkinning || !InSlicePMC.ProcMeshComp || InSlicePMC.SkinCache.Num() == 0 || SkinningMatrices.Num() == 0)
    {
        return;
    }

    // 2. TMap Lookup 최적화 (Pre-calculation)
    // ParallelFor 내부에서 TMap::Find는 느리므로, SectionIndex로 바로 접근 가능한 포인터 배열을 만듭니다.
    int32 MaxSectionIndex = 0;
    for (const auto& Pair : InSlicePMC.UpdateBuffers)
    {
        if (Pair.Key > MaxSectionIndex)
        {
            MaxSectionIndex = Pair.Key;
        }
    }

    // Lookup 테이블 생성 (인덱스 = SectionIndex, 값 = 버퍼 포인터)
    TArray<FProcMeshSectionBuffer*> FastBufferLookup;
    FastBufferLookup.SetNumZeroed(MaxSectionIndex + 1);

    for (auto& Pair : InSlicePMC.UpdateBuffers)
    {
        FastBufferLookup[Pair.Key] = &Pair.Value;
    }

    // 로컬 변수로 행렬 개수 캐싱 (루프 내 접근 비용 절약)
    const int32 MatrixCount = SkinningMatrices.Num();
    const FCachedSkinVertex* CacheData = InSlicePMC.SkinCache.GetData(); // Raw Pointer for Speed

    // 3. 병렬 연산 (ParallelFor)
    ParallelFor(InSlicePMC.SkinCache.Num(), [&](int32 i)
        {
            const FCachedSkinVertex& V = CacheData[i];

            // Lookup 테이블을 통해 O(1)로 버퍼 접근 (락 불필요: 읽기 전용 Lookup + 서로 다른 VertIndex 쓰기)
            if (!FastBufferLookup.IsValidIndex(V.SectionIndex)) return;
            FProcMeshSectionBuffer* Buffer = FastBufferLookup[V.SectionIndex];

            if (!Buffer || !Buffer->Vertices.IsValidIndex(V.VertIndex)) return;

            // 최종 위치/노멀 계산
            FVector FinalPos = FVector::ZeroVector;
            FVector FinalNormal = FVector::ZeroVector;
            FVector FinalTangentX = FVector::ZeroVector; // 탄젠트 누적 변수

            // Loop Unrolling 고려: 4번 고정이므로 컴파일러가 알아서 최적화하겠지만 명시적으로 깔끔하게 작성
            for (int32 w = 0; w < 4; ++w)
            {
                const float Weight = V.BoneWeights[w];

                // 유의미한 웨이트만 계산 (최적화 핵심)
                if (Weight > 0.01f)
                {
                    const int32 BoneIdx = V.BoneIndices[w];

                    // 행렬 인덱스 유효성 체크 (Bounds Check)
                    if (BoneIdx >= 0 && BoneIdx < MatrixCount)
                    {
                        const FMatrix& Mat = SkinningMatrices[BoneIdx];

                        // TransformPosition/Vector 연산
                        FinalPos += Mat.TransformPosition(V.InitialPos) * Weight;
                        FinalNormal += Mat.TransformVector(V.InitialNormal) * Weight;

                        // 탄젠트 벡터도 동일하게 회전
                        // Buffer->InitialTangents[V.VertIndex].TangentX 가 원본 벡터
                        FinalTangentX += Mat.TransformVector(Buffer->InitialTangents[V.VertIndex].TangentX) * Weight;
                    }
                }
            }

            // 결과 쓰기 (Thread-Safe: 각 스레드는 고유한 V.SectionIndex -> V.VertIndex에만 씀)
            Buffer->Vertices[V.VertIndex] = FinalPos;
            Buffer->Normals[V.VertIndex] = FinalNormal.GetSafeNormal(); // 정규화 필수

            // 탄젠트 정규화 및 업데이트
            // bFlipTangentY 값은 원본 그대로 유지
            bool bFlipY = Buffer->InitialTangents[V.VertIndex].bFlipTangentY;
            Buffer->Tangents[V.VertIndex] = FProcMeshTangent(FinalTangentX.GetSafeNormal(), bFlipY);
        });

    // 4. GPU 데이터 전송 (반드시 Main Thread에서 수행)
    // TMap 순회하며 변경된 버퍼 업데이트
    for (auto& Elem : InSlicePMC.UpdateBuffers)
    {
        const int32 SectionIdx = Elem.Key;
        const FProcMeshSectionBuffer& Buff = Elem.Value; // const reference로 복사 방지

        // 업데이트 수행
        InSlicePMC.ProcMeshComp->UpdateMeshSection_LinearColor(
            SectionIdx,
            Buff.Vertices,
            Buff.Normals,
            Buff.UVs,       // 백업된 UV 유지
            TArray<FVector2D>(), // UV1 (Empty)
            TArray<FVector2D>(), // UV2 (Empty)
            TArray<FVector2D>(), // UV3 (Empty)
            Buff.Colors,    // 백업된 Color 유지
            Buff.Tangents,  // 백업된 Tangent 유지
            false           // Collision Update 끔 (성능상 매우 중요)
        );
    }
}
