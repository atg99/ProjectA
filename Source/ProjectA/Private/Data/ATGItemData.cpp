// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/ATGItemData.h"

const FPrimaryAssetType UATGItemData::ATGItemData(TEXT("ATGItemData"));

FPrimaryAssetId UATGItemData::GetPrimaryAssetId() const
{
	// 자식 클래스 AssetId 통일
	return FPrimaryAssetId(ATGItemData, GetFName());
}
