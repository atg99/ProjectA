// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/CharacterAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "Interface/DamageableInterface.h"
#include "Utils/NetworkUtil.h"
#include "GameplayEffectExtension.h"

UCharacterAttributeSet::UCharacterAttributeSet()
{
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
}

//값이 반영되기전 강제
void UCharacterAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 최대 체력 넘지 않게 조절
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
}

void UCharacterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// 변경된 속성이 Damage일 경우에
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		float LocalDamage = GetDamage(); // 들어온 데미지 수치
		SetDamage(0.0f); // 소비했으니 0으로 초기화

		if (LocalDamage > 0.0f)
		{
			// 현재 체력 가져오기
			float CurrentHealth = GetHealth();

			// 체력 깎기, 방어력 계산 여기서 
			float NewHealth = FMath::Clamp(CurrentHealth - LocalDamage, 0.0f, GetMaxHealth());
			SetHealth(NewHealth);
			NET_LOG(FString::Printf(TEXT("%f"), NewHealth));
			// 맞은 대상
			AActor* TargetActor = Data.Target.GetAvatarActor();

			FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
			const FHitResult* HitResult = Context.GetHitResult();
			if (!ensure(HitResult))
			{
				return;
			}
			// 사망 처리
			if (NewHealth <= 0.0f)
			{
				if (IDamageableInterface* Damageable = Cast<IDamageableInterface>(TargetActor))
				{
					NET_LOG(FString::Printf(TEXT("HitLocation: %s ,CutNormal : %s"), *HitResult->ImpactPoint.ToString(), *HitResult->Normal.ToString()));
					Damageable->HandleDeath(*HitResult);
				}
			}
			else
			{
				//GameplayCue 실행
				if (Data.Target.AbilityActorInfo->IsNetAuthority())
				{
					
					FGameplayCueParameters CueParams;
					CueParams.EffectContext = Context;
					if (HitResult)
					{
						CueParams.Location = HitResult->ImpactPoint;
						CueParams.Normal = HitResult->ImpactNormal;
					}

					//속성별 태그 
					FGameplayTag CueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Hit.Physical");

					if (Data.EffectSpec.CapturedSourceTags.GetSpecTags().HasTag(FGameplayTag::RequestGameplayTag("Damage.Type.Fire")) // 시전자속성
						|| Data.EffectSpec.GetDynamicAssetTags().HasTag(FGameplayTag::RequestGameplayTag("Damage.Type.Fire"))) // 무기속성
					{
						CueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Hit.Fire");
					}
					else if (Data.EffectSpec.CapturedSourceTags.GetSpecTags().HasTag(FGameplayTag::RequestGameplayTag("Damage.Type.Ice"))
						|| Data.EffectSpec.GetDynamicAssetTags().HasTag(FGameplayTag::RequestGameplayTag("Damage.Type.Ice")))
					{
						CueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Hit.Ice");
					}

					// 큐 실행
					Data.Target.ExecuteGameplayCue(CueTag, CueParams);

					//피격 액션(경직)을 위한 Gameplay Event 전송
					// 단순 VFX가 아니라 상태이상(기절, 경직)
					//FGameplayEventData EventData;
					//EventData.EventTag = FGameplayTag::RequestGameplayTag("Event.Montage.HitReact");

					//// 타겟의 ASC에 이벤트를 보내서 GA_HitReact를 발동시킴
					//UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
					//	Data.Target.GetAvatarActor(),
					//	EventData.EventTag,
					//	EventData
					//);
				}
			}
		}
	}
}

void UCharacterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//클라에서 예측해서 서버와 값이 같아지면 OnRep이 실행되지 않기 때문에 REPNOTIFY_Always 옵션을 줘서 항상 실행되도록 설정
	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, Health, COND_None, REPNOTIFY_Always);
}

void UCharacterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	//서버에서 온 값을 GAS 시스템에 반영 서버에서 온 진짜 값과 클라이언트가 예측한 값을 비교해서, 예측 시스템이 꼬이지 않게 값을 올바르게 덮어씌워 주는 역할
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, Health, OldHealth);
}
