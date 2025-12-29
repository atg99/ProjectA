// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/AbilityTask_WaitMeleeTargetData.h"
#include "AbilitySystemComponent.h"
#include "Utils/NetworkUtil.h"

//팩토리 메서드(Factory Method)  스태틱(Static) 함수를 통해 객체를 생성하고 초기화해서 반환하는 패턴
UAbilityTask_WaitMeleeTargetData* UAbilityTask_WaitMeleeTargetData::WaitMeleeTargetData(UGameplayAbility* OwningAbility, FGameplayTag EventTag)
{
	if (GEngine)
	{
		UE_LOG(LogTemp, Error, TEXT("UAbilityTask_WaitMeleeTargetData::WaitMeleeTargetData"));
	}
	
	UAbilityTask_WaitMeleeTargetData* MeleeTask = NewAbilityTask<UAbilityTask_WaitMeleeTargetData>(OwningAbility);
	MeleeTask->TriggerTag = EventTag;
	return MeleeTask;
}

void UAbilityTask_WaitMeleeTargetData::Activate()
{
	if (!Ability || !AbilitySystemComponent.Get()) return;
	NET_LOG(TEXT(""));
	// 1. [Server] 클라이언트가 보낸 데이터 수신 대기
	// (Replicated Target Data는 SpecHandle을 통해 관리됩니다)
	AbilitySystemComponent->AbilityTargetDataSetDelegate(GetAbilitySpecHandle(), GetActivationPredictionKey()).AddUObject(this, &UAbilityTask_WaitMeleeTargetData::OnTargetDataReplicated);

	// 2. [Client & Server] 로컬 이벤트 리스너 등록
	// 클라이언트는 여기서 Notify가 보낸 이벤트를 잡습니다.
	// 서버는 만약 AI가 쓴다면 여기서 잡습니다(AI는 Notify가 서버에서 도니까).
	DelegateHandle = AbilitySystemComponent->AddGameplayEventTagContainerDelegate(
		FGameplayTagContainer(TriggerTag),
		FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UAbilityTask_WaitMeleeTargetData::OnLocalGameplayEvent)
	);
}

void UAbilityTask_WaitMeleeTargetData::OnDestroy(bool bInOwnerFinished)
{
	NET_LOG(TEXT(""));
	if (AbilitySystemComponent.Get())
	{
		//TargetData 델리게이트 해제
		AbilitySystemComponent->AbilityTargetDataSetDelegate(GetAbilitySpecHandle(), GetActivationPredictionKey()).RemoveAll(this);

		//Client 로컬 이벤트 리스너 해제 (반드시 해야 함)
		if (DelegateHandle.IsValid())
		{
			AbilitySystemComponent->RemoveGameplayEventTagContainerDelegate(
				FGameplayTagContainer(TriggerTag),
				DelegateHandle
			);
		}
	}

	Super::OnDestroy(bInOwnerFinished);
}

//Local
void UAbilityTask_WaitMeleeTargetData::OnLocalGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	if (!Payload) return;
	NET_LOG(TEXT(""));
	// TargetData 추출
	FGameplayAbilityTargetDataHandle TargetData = Payload->TargetData;

	// 데이터가 비었으면 HitResult로 생성
	if (TargetData.Num() == 0 && Payload->Target)
	{
		FGameplayAbilityTargetData_SingleTargetHit* NewData = new FGameplayAbilityTargetData_SingleTargetHit();
		NewData->HitResult.Location = Payload->Target->GetActorLocation();
		NewData->HitResult.HitObjectHandle = FActorInstanceHandle((AActor*)Payload->Target);
		NewData->HitResult.PhysMaterial = nullptr;
		TargetData.Add(NewData);
	}

	// FScopedPredictionWindow를 열어줘야 서버와 클라이언트 간의 예측 키가 동기화됨
	FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent.Get());

	//내가 클라이언트라면 -> 서버로 전송
	if (AbilitySystemComponent->GetOwnerRole() == ROLE_AutonomousProxy)
	{
		AbilitySystemComponent->CallServerSetReplicatedTargetData(
			GetAbilitySpecHandle(),
			GetActivationPredictionKey(),
			TargetData,
			FGameplayTag(),
			AbilitySystemComponent->ScopedPredictionKey
		);
	}
	//내가 서버(Host)라면 -> 전송할 필요 없이 바로 소비하고 브로드캐스트
	else if (AbilitySystemComponent->GetOwnerRole() == ROLE_Authority)
	{
		// 서버는 받은 데이터를 바로 ValidData로 쏘면 됨
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			ValidData.Broadcast(TargetData);
		}
	}

	//Client 로컬 타격감 즉시 처리 (소리/이펙트 예측 실행)
	//서버(Host)의 경우 위 2번에서 Broadcast했으므로 중복 방지를 위해 AutonomousProxy일 때만 하거나,
	//ValidData에 연결된 함수에서 IsLocallyControlled 체크를 해야 함.
	if (AbilitySystemComponent->GetOwnerRole() == ROLE_AutonomousProxy)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			ValidData.Broadcast(TargetData);
		}
	}
}

void UAbilityTask_WaitMeleeTargetData::OnTargetDataReplicated(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ActivationTag)
{
	NET_LOG(TEXT(""));
	//데이터를 소비했다고 표시 (메모리 정리)
	AbilitySystemComponent->ConsumeClientReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey());

	//Server 델리게이트 송출 -> GA에서 ApplyGameplayEffect 실행
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		ValidData.Broadcast(Data);
	}
}
