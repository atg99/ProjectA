// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA_MeleeCombo.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "AbilitySystemComponent.h"
#include "Utils/NetworkUtil.h"
#include "GAS/AbilityTask_WaitMeleeTargetData.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Data/ATGMeleeWeaponData.h"

UGA_MeleeCombo::UGA_MeleeCombo()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    CurrentComboIndex = 0;

    HitEventTag = FGameplayTag::RequestGameplayTag(FName("Event.Montage.Hit"));
}

void UGA_MeleeCombo::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    ////NET_LOG(TEXT(""));
	FName StartSection = ComboSections.IsValidIndex(CurrentComboIndex) ? ComboSections[CurrentComboIndex] : NAME_None;

    //CreatePlayMontageAndWaitProxy 같은 함수로 태스크를 만들면 태스크는 생성만 된 상태이고 일시정지 델리게이트를 다 연결한 뒤에 마지막에 반드시 ReadyForActivation을 호출
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MeleeMontage, 1.0f, StartSection, false);

    MontageTask->OnBlendOut.AddDynamic(this, &UGA_MeleeCombo::OnMontageEnded);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_MeleeCombo::OnMontageEnded);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_MeleeCombo::OnMontageEnded);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_MeleeCombo::OnMontageEnded);

    MontageTask->ReadyForActivation();

    CurrentComboIndex = 1;

    WaitForNextInput();

    WaitForHitTask();
}

void UGA_MeleeCombo::WaitForNextInput()
{
    // 사용자가 키를 누르는 것을 기다리는 태스크 생성
    UAbilityTask_WaitInputPress* InputTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false);
    InputTask->OnPress.AddDynamic(this, &UGA_MeleeCombo::OnInputPressed);
    InputTask->ReadyForActivation();
}

void UGA_MeleeCombo::WaitForHitTask()
{
    // HitCheck 태스크 실행
    UAbilityTask_WaitMeleeTargetData* HitTask = UAbilityTask_WaitMeleeTargetData::WaitMeleeTargetData(this, HitEventTag);
    HitTask->ValidData.AddDynamic(this, &UGA_MeleeCombo::OnHitReceived);
    HitTask->ReadyForActivation();
}

void UGA_MeleeCombo::OnInputPressed(float TimeWaited)
{
   // 몽타주 NotifyState에서 붙여준 태그가 있는지 확인
    bool bCanCombo = GetAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(ComboTag);

    if (bCanCombo && ComboSections.IsValidIndex(CurrentComboIndex))
    {
        USkeletalMeshComponent* Mesh = GetOwningComponentFromActorInfo();
        if (Mesh && Mesh->GetAnimInstance())
        {
            Mesh->GetAnimInstance()->Montage_JumpToSection(ComboSections[CurrentComboIndex], MeleeMontage);
            CurrentComboIndex++;

            if (CurrentComboIndex >= ComboSections.Num())
            {
                CurrentComboIndex = 0;
            }
        }

        WaitForNextInput();
    }
    else
    {
        // 콤보 타이밍이 아니면 입력 무시하거나 선입력 Queue 처리
        WaitForNextInput();
    }
}

void UGA_MeleeCombo::OnMontageEnded()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_MeleeCombo::OnHitReceived(const FGameplayAbilityTargetDataHandle& Data)
{
    // Client: 여기서 즉시 실행됨 (예측)
    // Server: 클라이언트 데이터가 도착하면 실행됨
    ////NET_LOG(TEXT(""));

    //GameplayEffect Spec 생성
    FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(MeleeGameplayEffectClass, 1.0f);
    //SpecHandle.Data.Co
    if (SpecHandle.IsValid())
    {
        // 데미지 수치 동적 변경 (SetByCaller)
        // SpecHandle.Data가 TSharedPtr이므로 .Get()으로 접근
        SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.Damage.Amount"), 40.0f);

        //무기 Tag추가 //character에서 어빌리티 등록할때 SourceObject 
        if (UATGMeleeWeaponData* MeleeWeaponData = Cast<UATGMeleeWeaponData>(GetCurrentSourceObject()))
        {
            SpecHandle.Data.Get()->AppendDynamicAssetTags(MeleeWeaponData->OwnedTags);
        }

        //클라 HitResult 복사
  /*      const FHitResult* HitResultPtr = Data.Get(0)->GetHitResult();
        if (HitResultPtr)
        {
            SpecHandle.Data->GetContext().AddHitResult(*HitResultPtr);
        }*/

        // UGameplayAbility::ApplyGameplayEffectSpecToTarget
        TArray<FActiveGameplayEffectHandle> AppliedEffects = ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, 
            CurrentActorInfo, 
            CurrentActivationInfo,
            SpecHandle,
            Data
        );
    }
}
