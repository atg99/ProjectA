// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/ATGItemData.h"

const FPrimaryAssetType UATGItemData::ATGItemData(TEXT("ATGItemData"));

FPrimaryAssetId UATGItemData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(ATGItemData, GetFName());
}
