// Fill out your copyright notice in the Description page of Project Settings.


#include "SliceSystemComponent.h"
#include "GameFramework/Character.h"
#include "Utils/SliceUtils.h"
#include "KismetProceduralMeshLibrary.h"
#include "ProceduralMeshComponent.h"

// 지오메트리 스크립트 필수 헤더
#include "Components/DynamicMeshComponent.h"
#include "GeometryScript/MeshAssetFunctions.h"
#include "GeometryScript/MeshBoneWeightFunctions.h" 
#include "GeometryScript/MeshBooleanFunctions.h"   
#include "GeometryScript/MeshRepairFunctions.h"     
#include "GeometryScript/SceneUtilityFunctions.h"


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
                // 원본 에셋을 복제하여 나만의 독립적인 에셋 생성
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
    USliceUtils::ConvertBoneToProcMesh_2(Mesh, TargetBone, PMC_Stump);

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
        PMC_Stump, HitLocation, CutNormal, true,
        PMC_Debris, EProcMeshSliceCapOption::CreateNewSectionForCap,
        CapMaterial
    );

    //// 3. 위치 재조정 및 부착

    // [Debris 설정] : 잘려나간 부위
    // Debris는 TargetBone(물리 시뮬레이션 될 본)에 붙입니다.
    PMC_Debris->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetBone);
    PMC_Debris->SetRelativeTransform(FTransform::Identity); // 오프셋 없이 본 위치에 일치시킴

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
    USkeletalMeshComponent* SKMesh = Character->GetMesh();

    // =================================================================================
    // 1. 본 데이터 추출 (기존 ConvertBoneToProcMesh 대체)
    // =================================================================================
    // 복잡한 버퍼 파싱 없이, 이 함수 하나면 해당 본(TargetBone)의 메쉬만 딱 떼어옵니다.

    //UDynamicMesh* StumpMesh = DMC_Stump->GetDynamicMesh();
    //StumpMesh->Reset();

    //FGeometryScriptCopyMeshFromComponentOptions ComponentOptions;
    //ComponentOptions.bWantTangents = true;
    //ComponentOptions.bWantNormals = true;

    //UGeometryScriptLibrary_SceneUtilityFunctions::CopyMeshFromComponent(
    //    TargetMesh,
    //    SourceComp, // USkeletalMeshComponent* (Asset 아님!)
    //    ComponentOptions,
    //    RequestedLOD,
    //    Outcome,
    //    nullptr
    //);

    //// 원래부터 뚫려있던 구멍(어깨 연결부 등) 메우기
    //UGeometryScriptLibrary_MeshRepairFunctions::ApplyMeshCloseHoles(StumpMesh);

    //// 머터리얼 복사 (슬롯 인덱스 유지)
    //int32 MaterialCount = SKMesh->GetNumMaterials();
    //for (int32 i = 0; i < MaterialCount; ++i)
    //{
    //    DMC_Stump->SetMaterial(i, SKMesh->GetMaterial(i));
    //    DMC_Debris->SetMaterial(i, SKMesh->GetMaterial(i));
    //}


    //// =================================================================================
    //// 2. 자르기 (기존 SliceProceduralMesh 대체)
    //// =================================================================================
    //// 로컬 좌표계 계산: StumpMesh가 현재 본의 로컬 좌표계(Ref Pose)에 있다고 가정
    //// HitLocation을 해당 본의 공간으로 변환해야 함

    //// Stump는 이제 TargetBone 위치에 붙을 것이므로, TargetBone 기준으로 변환
    //FTransform BoneTransform = SKMesh->GetSocketTransform(TargetBone);
    //FVector LocalCutPos = BoneTransform.InverseTransformPosition(HitLocation);
    //FVector LocalCutNormal = BoneTransform.InverseTransformVector(HitNormal);

    //UDynamicMesh* DebrisMesh = DMC_Debris->GetDynamicMesh();
    //DebrisMesh->Reset();

    //// [핵심] 자르기 수행
    //UGeometryScriptLibrary_MeshBooleanFunctions::ApplyMeshPlaneCut(
    //    StumpMesh,
    //    FTransform::Identity,
    //    LocalCutPos,
    //    LocalCutNormal,
    //    true, // bFillHoles (캡 생성 - 훨씬 안정적임)
    //    true, // 잘린 조각(Debris) 분리
    //    DebrisMesh, // Debris가 들어갈 메쉬
    //    true // Debris Export 활성화
    //);

    //// 캡 머터리얼 할당 (옵션)
    //if (CapMaterial)
    //{
    //    // DynamicMesh는 자른 단면에 새로운 Material ID를 부여합니다.
    //    // 보통 마지막 ID이므로 추가해줍니다.
    //    DMC_Stump->SetMaterial(MaterialCount, CapMaterial);
    //    DMC_Debris->SetMaterial(MaterialCount, CapMaterial);
    //}

    //// =================================================================================
    //// 3. 부착 및 스키닝 흉내 (기존 로직과 동일)
    //// =================================================================================

    //// [Stump 설정]
    //// Stump는 TargetBone의 RefPose 형태로 추출되었습니다.
    //// 따라서 TargetBone에 Attach하면 애니메이션을 따라갑니다. (Rigid Binding)

    //// 부모 본을 찾습니다.
    //FName ParentBone = SKMesh->GetParentBone(TargetBone);

    //// 기존 코드의 로직: ParentBone에 붙이고 오프셋을 줌
    //// DMC도 동일하게 처리 가능하지만, CopyBoneFrom... 함수가 RefPose 중심으로 가져왔다면
    //// 그냥 TargetBone에 붙여도 무방할 수 있습니다. 
    //// 하지만 기존 로직(Parent에 붙임)을 따르자면:

    //if (ParentBone != NAME_None)
    //{
    //    // 1. 부모 본에 부착
    //    DMC_Stump->AttachToComponent(SKMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ParentBone);

    //    // 2. 오프셋 적용 (TargetBone의 로컬 Transform)
    //    // RefSkeleton에서 TargetBone의 Local Transform을 가져옴
    //    int32 BoneIndex = SKMesh->GetBoneIndex(TargetBone);
    //    if (BoneIndex != INDEX_NONE)
    //    {
    //        FTransform RefBoneTransform = SKMesh->GetSkeletalMeshAsset()->GetRefSkeleton().GetRefBonePose()[BoneIndex];
    //        DMC_Stump->SetRelativeTransform(RefBoneTransform);
    //    }
    //}
    //else
    //{
    //    DMC_Stump->AttachToComponent(SKMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetBone);
    //}

    //// [Debris 설정]
    //// Debris는 물리 적용
    //// 월드 위치로 다시 맞춤 (Attach하면 로컬로 바뀌므로 주의)
    //DMC_Debris->SetWorldTransform(BoneTransform); // 대략적인 시작 위치
    //DMC_Debris->SetVisibility(true);
    //DMC_Debris->SetSimulatePhysics(true);
    //DMC_Debris->AddImpulse(-HitNormal * ImpulsePower, NAME_None, true);

    //DMC_Stump->SetVisibility(true);

    // =================================================================================
    // 4. 원본 마스킹 (기존 로직 유지)
    // =================================================================================
    // 해당 본 숨기기 (엔진 내장 함수 사용 권장)
    SKMesh->HideBoneByName(TargetBone, EPhysBodyOp::PBO_None);

    // 물리 끄기 (Dangling 방지)
    SKMesh->SetAllBodiesBelowSimulatePhysics(TargetBone, false);
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

