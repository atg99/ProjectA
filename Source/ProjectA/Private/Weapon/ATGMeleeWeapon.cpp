// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ATGMeleeWeapon.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Utils/NetworkUtil.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "Animation/BuiltInAttributeTypes.h" 
#include "ATGPlayerCharacter.h"

AATGMeleeWeapon::AATGMeleeWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

    HitEventTag = FGameplayTag::RequestGameplayTag(FName("Event.Montage.Hit"));
}

void AATGMeleeWeapon::StartHitCheck()
{
    //Cast<APawn>(GetOwner())->IsLocallyControlled();
    //CastChecked<APawn>(GetOwner())->IsLocallyControlled();
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character)
    {
        return;
    }

    //로컬 실행일때만
    if (!Character->IsLocallyControlled())
    {
        return;
    }

    IgnoreActors.Empty();
    IgnoreActors.Add(GetOwner()); 

    if (Mesh)
    {
        PreviousStartLocation = Mesh->GetSocketLocation(StartSocketName);
        PreviousEndLocation = Mesh->GetSocketLocation(EndSocketName);
    }

    
    if (Character->GetMesh() && Character->GetMesh()->GetAnimInstance())
    {
        UAnimInstance* AnimInst = Character->GetMesh()->GetAnimInstance();
        UAnimMontage* CurrentMontage = AnimInst->GetCurrentActiveMontage();

        if (CurrentMontage)
        {
            PreviousMontagePosition = AnimInst->Montage_GetPosition(CurrentMontage);
        }
    }

    IsFirstFrame = true;
}

void AATGMeleeWeapon::TickHitCheck()
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character)
    {
        return;
    }

    //로컬 실행일때만
    if (!Character->IsLocallyControlled())
    {
        return;
    }

    if (!Mesh) return;

    TrajectoryInterpolationbySubFrame();
    return;

    ////소유자 확인
    //AActor* OwnerActor = GetOwner();
    //if (!OwnerActor) 
    //{
    //    return;
    //}
    //APawn* OwnerPawn = Cast<APawn>(Owner);
    //if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
    //{
    //    return;
    //}

    ////현재 소켓 위치 가져오기
    //FVector CurrentStart = Mesh->GetSocketLocation(StartSocketName);
    //FVector CurrentEnd = Mesh->GetSocketLocation(EndSocketName);

    ////트레이스 수행
    //TArray<FHitResult> OutHits;
    //FVector Center = (CurrentStart + CurrentEnd) * 0.5f;
    //FRotator Rotation = Mesh->GetSocketRotation(StartSocketName);
    //FVector HalfSize = FVector((CurrentStart - CurrentEnd).Size() * 0.5f, TraceRadius, TraceRadius);

    ////Start와 End 파라미터를 사용하지 않고 Box의 위치와 회전을 직접 계산하는 방식이
    ////무기 회전에 따른 정확한 판정에 더 유리할 수 있습니다.

    ////무기 끝부분(타격점)의 궤적을 따라 SphereTrace
    //bool bHit = UKismetSystemLibrary::SphereTraceMulti(
    //    this,
    //    PreviousEndLocation, //지난 프레임의 칼끝
    //    CurrentEnd,          //현재 프레임의 칼끝
    //    TraceRadius,
    //    UEngineTypes::ConvertToTraceType(ECC_Pawn),
    //    false,
    //    IgnoreActors.Array(),
    //    EDrawDebugTrace::ForDuration,
    //    OutHits,
    //    true, FLinearColor::Red, FLinearColor::Green, 5.f
    //);

    //if (bHit)
    //{
    //    for (const FHitResult& Hit : OutHits)
    //    {
    //        ProcessHit(Hit);
    //    }
    //}

    ////현재 위치를 다음 프레임의 이전 위치로 저장
    //PreviousStartLocation = CurrentStart;
    //PreviousEndLocation = CurrentEnd;
}

void AATGMeleeWeapon::EndHitCheck()
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character)
    {
        return;
    }

    //로컬 실행일때만
    if (!Character->IsLocallyControlled())
    {
        return;
    }
    IgnoreActors.Empty();
}

void AATGMeleeWeapon::ProcessHit(const FHitResult& HitResult)
{
    AActor* HitActor = HitResult.GetActor();
    if (!HitActor || IgnoreActors.Contains(HitActor)) return;
    NET_LOG(TEXT(""));
    IgnoreActors.Add(HitActor);
    AActor* OwnerActor = GetOwner();

    FGameplayEventData Payload;
    Payload.Instigator = Owner;
    Payload.Target = HitActor;
    Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(HitResult);

    //로컬 ASC에만 이벤트를 던짐
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, HitEventTag, Payload);
}

void AATGMeleeWeapon::TrajectoryInterpolationbySubFrame()
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;
    UAnimInstance* AnimInst = Character->GetMesh()->GetAnimInstance();
    if (!AnimInst) return;
    UAnimMontage* Montage = AnimInst->GetCurrentActiveMontage();
    if (!Montage) return;

    //시간 델타 계산
    float CurrentPos = AnimInst->Montage_GetPosition(Montage);
    float DeltaTime = CurrentPos - PreviousMontagePosition;

    FVector SocketStart = Mesh->GetSocketLocation(StartSocketName);
    FVector SocketEnd = Mesh->GetSocketLocation(EndSocketName);
    DrawDebugPoint(GetWorld(), SocketStart, 10.f, FColor::Magenta, false, 5.f);
    DrawDebugPoint(GetWorld(), SocketEnd, 10.f, FColor::Magenta, false, 5.f);
    DrawDebugLine(GetWorld(), SocketStart, SocketEnd, FColor::Cyan, false, 5.f);

    // 시간이 튀거나 0이면 초기화
    if (FMath::IsNearlyZero(DeltaTime) || DeltaTime < 0.f)
    {
        PreviousMontagePosition = CurrentPos;
        // 튀는 현상 방지: 현재 실제 소켓 위치로 초기화
        if (Mesh)
        {
            PrevBladeState.Start = SocketStart;
            PrevBladeState.End = SocketEnd;
        }
        return;
    }

    float CorrectLocalTime = 0.0f;
    const UAnimSequence* AnimSeq = GetCurrentAnimSequenceWithTime(AnimInst, Montage, CorrectLocalTime);

    if (!AnimSeq) return;

    FTransform MeshTrans = Character->GetMesh()->GetComponentTransform();
    float StepSize = DeltaTime / (float)SubFrameSteps;

    //FTransform HandRTransform;
    //if (AATGPlayerCharacter* ATGCharacter = Cast<AATGPlayerCharacter>(Character))
    //{
    //    if (ATGCharacter->GetSlaveMesh())
    //    {
    //        HandRTransform = ATGCharacter->GetSlaveMesh()->GetSocketTransform(TEXT("Socket_Ref_Hand"), ERelativeTransformSpace::RTS_World);
    //    }
    //}
   
    // 과거 -> 현재
    for (int32 i = 1; i <= SubFrameSteps; ++i)
    {
        float SampleTime = CorrectLocalTime - DeltaTime + (StepSize * i);

        // 커브에서 위치 가져오기 (Local Space)
        FVector LocalStart = GetVectorFromCurves(AnimSeq, CurveName_Start, SampleTime);
        FVector LocalEnd = GetVectorFromCurves(AnimSeq, CurveName_End, SampleTime);

        // 월드로 변환
        FBladeState CurrBladeState;
        CurrBladeState.Start = MeshTrans.TransformPosition(LocalStart);
        CurrBladeState.End = MeshTrans.TransformPosition(LocalEnd);

        DrawDebugPoint(GetWorld(), CurrBladeState.Start, 10.f, FColor::Yellow, false, 5.f);
        DrawDebugPoint(GetWorld(), CurrBladeState.End, 10.f, FColor::Yellow, false, 5.f);

        //삼각형
        TArray<FBladeState> StartEndLocs;
        if (IsFirstFrame)
        {
            IsFirstFrame = false;
        }
        else
        {
            StartEndLocs.Add(FBladeState(PrevBladeState.Start, CurrBladeState.Start));
            StartEndLocs.Add(FBladeState(PrevBladeState.End, CurrBladeState.End));
            StartEndLocs.Add(FBladeState(PrevBladeState.End, CurrBladeState.Start));
        }
        StartEndLocs.Add(FBladeState(CurrBladeState.Start, CurrBladeState.End));

        for (const FBladeState& Locs : StartEndLocs)
        {
            FHitResult Hit;
            FCollisionQueryParams Params;
            Params.AddIgnoredActor(GetOwner());
            Params.AddIgnoredActors(IgnoreActors.Array());
            Params.bReturnPhysicalMaterial = true;
            Params.bTraceComplex = true;

            //Params.bReturnFaceIndex;
            bool bHit = false;

            DrawDebugLine(GetWorld(), Locs.Start, Locs.End, FColor::Red, false, 5.f);
            
            bHit = GetWorld()->LineTraceSingleByChannel(Hit, Locs.Start, Locs.End, ECC_GameTraceChannel2, Params);
            if (bHit)
            {
                DrawDebugPoint(GetWorld(), Hit.Location, 50.f, FColor::Green, false, 5.f);
                //Cut Normal
                FVector BladeNormal = (CurrBladeState.Start - CurrBladeState.End).GetSafeNormal();
                FVector SwingNormal = (CurrBladeState.Start - PrevBladeState.Start).GetSafeNormal();
                FVector CutNormal = FVector::CrossProduct(BladeNormal, SwingNormal).GetSafeNormal();

                //Normal에 저장
                Hit.Normal = CutNormal;
                NET_LOG(FString::Printf(TEXT("CutNormal : %s"), *CutNormal.ToString()));
                ProcessHit(Hit);
            }
        }

        // 상태 업데이트
        PrevBladeState = CurrBladeState;
        //for (int32 Seg = 0; Seg <= BladeSegmentCount; ++Seg)
        //{
        //    float Alpha = (float)Seg / (float)BladeSegmentCount;

        //    FVector TraceStart = FMath::Lerp(PrevBladeState.Start, PrevBladeState.End, Alpha);
        //    FVector TraceEnd = FMath::Lerp(CurrBladeState.Start, CurrBladeState.End, Alpha);

        //    FHitResult Hit;
        //    FCollisionQueryParams Params;
        //    Params.AddIgnoredActor(GetOwner());
        //    Params.AddIgnoredActors(IgnoreActors.Array());
        //    Params.bReturnPhysicalMaterial = true;

        //    bool bHit = GetWorld()->LineTraceSingleByChannel(
        //        Hit, TraceStart, TraceEnd, ECC_GameTraceChannel1, Params
        //    );

        //    DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 0.5f);

        //    if (bHit)
        //    {
        //        ProcessHit(Hit);
        //    }
        //}


    }

    PreviousMontagePosition = CurrentPos;
}

FVector AATGMeleeWeapon::GetVectorFromCurves(const UAnimSequenceBase* AnimSeq, FName BaseName, float Time) const
{
    if (!AnimSeq) return FVector::ZeroVector;

    FAnimExtractContext Context;
    Context.CurrentTime = Time;

    //3번 읽기
    return FVector(
        AnimSeq->EvaluateCurveData(FName(BaseName.ToString() + "x"), Context),
        AnimSeq->EvaluateCurveData(FName(BaseName.ToString() + "y"), Context),
        AnimSeq->EvaluateCurveData(FName(BaseName.ToString() + "z"), Context)
    );
}

const UAnimSequence* AATGMeleeWeapon::GetCurrentAnimSequenceWithTime(UAnimInstance* AnimInst, UAnimMontage* Montage, float& OutLocalTime)
{
    if (!AnimInst || !Montage) return nullptr;

    float CurrentPos = AnimInst->Montage_GetPosition(Montage);

    const TArray<FSlotAnimationTrack>& SlotTracks = Montage->SlotAnimTracks;
    if (SlotTracks.Num() <= 0) return nullptr;

    const FSlotAnimationTrack& MainTrack = SlotTracks[0];

    for (const FAnimSegment& Segment : MainTrack.AnimTrack.AnimSegments)
    {
        float LocalTime = 0.f;
        if (UAnimSequenceBase* AnimSeqBase = Segment.GetAnimationData(CurrentPos, LocalTime))
        {
            OutLocalTime = LocalTime;
            return Cast<UAnimSequence>(AnimSeqBase);
        }
    }

    return nullptr;
}
