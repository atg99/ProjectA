// Fill out your copyright notice in the Description page of Project Settings.


#include "SliceSystemComponent.h"
#include "GameFramework/Character.h"
#include "Utils/SliceUtils.h"
#include "KismetProceduralMeshLibrary.h"
#include "ProceduralMeshComponent.h"

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
    Mesh->BreakConstraint(FVector::ZeroVector, HitLocation, TargetBone);
    Mesh->AddImpulse(-HitNormal * ImpulsePower, TargetBone, false);

    // 5. 마스킹
    USliceUtils::MaskTargetBoneOnly(Character->GetMesh(), TargetBone);

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

