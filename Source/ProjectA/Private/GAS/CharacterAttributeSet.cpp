// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/CharacterAttributeSet.h"
#include "Net/UnrealNetwork.h"

UCharacterAttributeSet::UCharacterAttributeSet()
{
	InitHealth(100.f);	
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
