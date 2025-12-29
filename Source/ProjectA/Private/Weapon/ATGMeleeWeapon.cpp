// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ATGMeleeWeapon.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Utils/NetworkUtil.h"

AATGMeleeWeapon::AATGMeleeWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

    HitEventTag = FGameplayTag::RequestGameplayTag(FName("Event.Montage.Hit"));
}

void AATGMeleeWeapon::StartHitCheck()
{
    IgnoreActors.Empty();
    IgnoreActors.Add(GetOwner()); 

    if (Mesh)
    {
        PreviousStartLocation = Mesh->GetSocketLocation(StartSocketName);
        PreviousEndLocation = Mesh->GetSocketLocation(EndSocketName);
    }
}

void AATGMeleeWeapon::TickHitCheck()
{
    if (!Mesh) return;

    //소유자 확인
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor) 
    {
        return;
    }
    APawn* OwnerPawn = Cast<APawn>(Owner);
    if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
    {
        return;
    }

    //현재 소켓 위치 가져오기
    FVector CurrentStart = Mesh->GetSocketLocation(StartSocketName);
    FVector CurrentEnd = Mesh->GetSocketLocation(EndSocketName);

    //트레이스 수행
    TArray<FHitResult> OutHits;
    FVector Center = (CurrentStart + CurrentEnd) * 0.5f;
    FRotator Rotation = Mesh->GetSocketRotation(StartSocketName);
    FVector HalfSize = FVector((CurrentStart - CurrentEnd).Size() * 0.5f, TraceRadius, TraceRadius);

    //Start와 End 파라미터를 사용하지 않고 Box의 위치와 회전을 직접 계산하는 방식이
    //무기 회전에 따른 정확한 판정에 더 유리할 수 있습니다.

    //무기 끝부분(타격점)의 궤적을 따라 SphereTrace
    bool bHit = UKismetSystemLibrary::SphereTraceMulti(
        this,
        PreviousEndLocation, //지난 프레임의 칼끝
        CurrentEnd,          //현재 프레임의 칼끝
        TraceRadius,
        UEngineTypes::ConvertToTraceType(ECC_Pawn),
        false,
        IgnoreActors.Array(),
        EDrawDebugTrace::ForDuration,
        OutHits,
        true, FLinearColor::Red, FLinearColor::Green, 5.f
    );

    if (bHit)
    {
        for (const FHitResult& Hit : OutHits)
        {
            ProcessHit(Hit);
        }
    }

    //현재 위치를 다음 프레임의 이전 위치로 저장
    PreviousStartLocation = CurrentStart;
    PreviousEndLocation = CurrentEnd;
}

void AATGMeleeWeapon::EndHitCheck()
{
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
