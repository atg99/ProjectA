// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Title/NetworkGameInstanceSubsystem.h"
#include "MarketSubsystem.generated.h"


USTRUCT(BlueprintType)
struct FMarketItemMetadata
{
    GENERATED_BODY()

};

// 매물 아이템
USTRUCT(BlueprintType)
struct FMarketListingItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 listing_id = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 seller_uid = 0;

    UPROPERTY(BlueprintReadOnly)
    FString seller_name;

    UPROPERTY(BlueprintReadOnly)
    FString primary_asset_id;

    UPROPERTY(BlueprintReadOnly)
    int32 qty = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 price = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 status = 0;

    UPROPERTY(BlueprintReadOnly)
    FMarketItemMetadata item_metadata;

    UPROPERTY(BlueprintReadOnly)
    FString created_at;

    // my-listings용 (nullable 대응 FString)
    UPROPERTY(BlueprintReadOnly)
    FString sold_at;
};

// 페이지네이션 정보
USTRUCT(BlueprintType)
struct FMarketPagination
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 page = 1;

    UPROPERTY(BlueprintReadOnly)
    int32 limit = 20;

    UPROPERTY(BlueprintReadOnly)
    int32 total = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 total_pages = 0;
};

USTRUCT()
struct FMarketListingsResponse
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMarketListingItem> data;

    UPROPERTY()
    FMarketPagination pagination;
};

// 메시지
USTRUCT()
struct FMarketMessageResponse
{
    GENERATED_BODY()

    UPROPERTY()
    FString message;

    UPROPERTY()
    int32 listing_id = 0; // POST 성공 시
};

UENUM(BlueprintType)
enum class EMarketSortType : uint8
{
    None = 0		    UMETA(DisplayName = "None"),
    PriceASC = 1		UMETA(DisplayName = "PriceASC"),
    PriceDESC = 2	    UMETA(DisplayName = "PriceDESC"),
    CreatedAtDESC = 3	UMETA(DisplayName = "CreatedAtDESC"),
};

UENUM(BlueprintType)
enum class EListingStatusType : uint8
{
    None = 0		UMETA(DisplayName = "None"),
    Active = 1		UMETA(DisplayName = "Active"),
    Sold = 2	    UMETA(DisplayName = "Sold"),
    History = 3 	UMETA(DisplayName = "History"),
};

/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMarketListingsUpdated, const TArray<FMarketListingItem>&, Items, const FMarketPagination&, Pagination);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMyListingsUpdated, const TArray<FMarketListingItem>&, Items, const FMarketPagination&, Pagination);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLookupItem, const FMarketListingItem&, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMarketMessaged, const FMarketMessageResponse&, MessageResponse);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveAndLoad, bool, bWasSuccessful);

class APlayerController;

UCLASS()
class PROJECTA_API UMarketSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
    // 시장에 등록된 활성 매물(`status = 0`)들을 조회합니다.
    UFUNCTION(BlueprintCallable)
    void RequestMarketListings(int32 Page = 1, int32 Limit = 20, EMarketSortType SortType = EMarketSortType::CreatedAtDESC, FString Keyword = "");

    // 자신이 등록한 매물 목록을 조회합니다.
    UFUNCTION(BlueprintCallable)
    void RequestMyListings(EListingStatusType StatusType, int32 Page, int32 Limit, FString Keyword);

    // 특정 매물의 상세 정보를 조회합니다.
    UFUNCTION(BlueprintCallable)
    void RequestLookupListing(int32 LisingID);

    // 인벤토리의 아이템을 시장에 등록합니다. 아이템은 인벤토리에서 제거됩니다.
    UFUNCTION(BlueprintCallable)
    void RequestRegisterListing(int32 ItemDBID, APlayerController* PC, int32 Price = 100, int32 Qty = 1);

    // 등록된 매물을 구매합니다. 구매자의 골드가 차감되고 아이템은 구매자의 창고(Stash)로 지급됩니다.
    UFUNCTION(BlueprintCallable)
    void RequestPurchaseListing(int32 LisingID);

    // 판매 중인 매물을 취소합니다. 아이템은 판매자의 창고(Stash)로 회수됩니다.
    UFUNCTION(BlueprintCallable)
    void RequestCancelListing(int32 LisingID);

    // 창고 인벤토리 저장 BP
    UFUNCTION(BlueprintCallable)
    void SaveStashData(AController* Controller);

    // C++ Callback 내부 호출용
    void SaveStashData(AController* Controller, FOnNetworkResponse Callback);

    // 창고 인벤토리 저장 BP
    UFUNCTION(BlueprintCallable)
    void LoadStashData(AController* Controller);

    void LoadStashData(AController* Controller, FOnNetworkResponse Callback);

    // 저장하고 끝나면 로드 
    UFUNCTION(BlueprintCallable)
    void SaveLoadStashData(AController* Controller);

    UPROPERTY(BlueprintAssignable)
    FOnMarketListingsUpdated OnMarketItemsUpdated;

    UPROPERTY(BlueprintAssignable)
    FOnMyListingsUpdated OnMyItemsUpdated;

    UPROPERTY(BlueprintAssignable)
    FOnLookupItem OnLookupItem;

    UPROPERTY(BlueprintAssignable)
    FOnMarketMessaged OnItemRegistered;

    UPROPERTY(BlueprintAssignable)
    FOnMarketMessaged OnItemPurchased;

    UPROPERTY(BlueprintAssignable)
    FOnMarketMessaged OnItemCanceled;

    UPROPERTY(BlueprintAssignable)
    FOnSaveStashRequstResult OnStashSaveResult;

    UPROPERTY(BlueprintAssignable)
    FOnSaveAndLoad OnStashSaveAndLoad;

};
