// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA_MeleeCombo.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "AbilitySystemComponent.h"
#include "Utils/NetworkUtil.h"

UGA_MeleeCombo::UGA_MeleeCombo()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    CurrentComboIndex = 0;
}

void UGA_MeleeCombo::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    NET_LOG(TEXT(""));
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
}

void UGA_MeleeCombo::WaitForNextInput()
{
    // 사용자가 키를 누르는 것을 기다리는 태스크 생성
    UAbilityTask_WaitInputPress* InputTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false);
    InputTask->OnPress.AddDynamic(this, &UGA_MeleeCombo::OnInputPressed);
    InputTask->ReadyForActivation();
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
