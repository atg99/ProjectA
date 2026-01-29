// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class PROJECTA_API NetworkUtil
{
public:
	NetworkUtil();
	~NetworkUtil();

	static void Log(UWorld* World = nullptr, const FString& CallInfo = "", const FString& InString="");
};

#define NET_LOG(String)		NetworkUtil::Log(GetWorld(), ANSI_TO_TCHAR(__FUNCTION__), String)
#define NET_LOG2(String)		NetworkUtil::Log(nullptr, ANSI_TO_TCHAR(__FUNCTION__), String)