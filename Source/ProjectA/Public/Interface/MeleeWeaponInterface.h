// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MeleeWeaponInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UMeleeWeaponInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PROJECTA_API IMeleeWeaponInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	// 공격 시작 
	virtual void StartHitCheck() = 0;

	// 매 프레임 판정 수행
	virtual void TickHitCheck() = 0;

	// 공격 종료
	virtual void EndHitCheck() = 0;


};
