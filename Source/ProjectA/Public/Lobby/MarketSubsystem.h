// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Title/NetworkGameInstanceSubsystem.h"
#include "MarketSubsystem.generated.h"


USTRUCT(BlueprintType)
struct FUserProfile 
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString username;

    UPROPERTY(BlueprintReadOnly)
    int32 level;

    UPROPERTY(BlueprintReadOnly)
    int32 exp;

    UPROPERTY(BlueprintReadOnly)
    int32 gold;
};

USTRUCT(BlueprintType)
struct FSellToSystemResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString message;

    UPROPERTY(BlueprintReadOnly)
    int32 earned_gold;

    UPROPERTY(BlueprintReadOnly)
    int32 current_gold;
};


USTRUCT(BlueprintType)
struct FMarketItemMetadata
{
    GENERATED_BODY()

};

// �Ź� ������
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

    // my-listings�� (nullable ���� FString)
    UPROPERTY(BlueprintReadOnly)
    FString sold_at;
};

// ���������̼� ����
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

USTRUCT(BlueprintType)
struct FMarketListingsResponse
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TArray<FMarketListingItem> data;

    UPROPERTY(BlueprintReadOnly)
    FMarketPagination pagination;
};

USTRUCT(BlueprintType)
struct FTradeSellItemRequest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString source_type = "stash";

    UPROPERTY(BlueprintReadWrite)
    int32 item_entry_id = 0;

    UPROPERTY(BlueprintReadWrite)
    int32 qty = 0;
};

USTRUCT(BlueprintType)
struct FTradeBuyItemRequest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString primary_asset_id;

    UPROPERTY(BlueprintReadWrite)
    int32 qty = 0;
};

USTRUCT(BlueprintType)
struct FTradeItemRequest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TArray<FTradeSellItemRequest> sell_items;

    UPROPERTY(BlueprintReadWrite)
    TArray<FTradeBuyItemRequest> buy_items;
};

USTRUCT(BlueprintType)
struct FTradeBuyItemResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString primary_asset_id;

    UPROPERTY(BlueprintReadOnly)
    int32 qty = 0;
};

USTRUCT(BlueprintType)
struct FTradeResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString message;

    UPROPERTY(BlueprintReadOnly)
    int32 earned_gold = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 spent_gold = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 current_gold = 0;

    UPROPERTY(BlueprintReadOnly)
    TArray<FTradeBuyItemResult> bought_items;
};

// �޽���
USTRUCT(BlueprintType)
struct FMarketMessageResponse
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString message;

    UPROPERTY(BlueprintReadOnly)
    int32 listing_id = 0; // POST ���� ��
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


UENUM(BlueprintType)
enum class EStorageSourceType : uint8
{
    None = 0		UMETA(DisplayName = "None"),
    Inventory = 1	UMETA(DisplayName = "Inventory"),
    Stash = 2	    UMETA(DisplayName = "Stash"),
};

/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLoadUserProfile, const FUserProfile&, UserProfile, const int32, Code);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSellToSystem, const FSellToSystemResult&, SellToSystemResult, const int32, Code);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMarketListingsUpdated, const TArray<FMarketListingItem>&, Items, const FMarketPagination&, Pagination);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMyListingsUpdated, const TArray<FMarketListingItem>&, Items, const FMarketPagination&, Pagination);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLookupItem, const FMarketListingItem&, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMarketMessaged, const FMarketMessageResponse&, MessageResponse, int32, Code);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTradeResult, const FTradeResult&, TradeResult, int32, Code);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveAndLoad, bool, bWasSuccessful);

class APlayerController;

UCLASS()
class PROJECTA_API UMarketSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:

    UFUNCTION(BlueprintCallable)
    void RequestUserProfile();

    UFUNCTION(BlueprintCallable)
    void RequestSellToSystem(EStorageSourceType SourceType, int32 ItemDBID, int32 Qty);

    UFUNCTION(BlueprintCallable)
    void RequestTradeItems(const FTradeItemRequest& TradeItemRequest);

    // ���忡 ��ϵ� Ȱ�� �Ź�(`status = 0`)���� ��ȸ�մϴ�.
    UFUNCTION(BlueprintCallable)
    void RequestMarketListings(int32 Page = 1, int32 Limit = 20, EMarketSortType SortType = EMarketSortType::CreatedAtDESC, FString Keyword = "");

    // �ڽ��� ����� �Ź� ����� ��ȸ�մϴ�.
    UFUNCTION(BlueprintCallable)
    void RequestMyListings(EListingStatusType StatusType, int32 Page, int32 Limit, FString Keyword);

    // Ư�� �Ź��� �� ������ ��ȸ�մϴ�.
    UFUNCTION(BlueprintCallable)
    void RequestLookupListing(int32 LisingID);

    // �κ��丮�� �������� ���忡 ����մϴ�. �������� �κ��丮���� ���ŵ˴ϴ�.
    UFUNCTION(BlueprintCallable)
    void RequestRegisterListing(int32 ItemDBID, APlayerController* PC, int32 Price = 100, int32 Qty = 1);

    // ��ϵ� �Ź��� �����մϴ�. �������� ��尡 �����ǰ� �������� �������� â��(Stash)�� ���޵˴ϴ�.
    UFUNCTION(BlueprintCallable)
    void RequestPurchaseListing(int32 LisingID);

    // �Ǹ� ���� �Ź��� ����մϴ�. �������� �Ǹ����� â��(Stash)�� ȸ���˴ϴ�.
    UFUNCTION(BlueprintCallable)
    void RequestCancelListing(int32 LisingID);

    // â�� �κ��丮 ���� BP
    UFUNCTION(BlueprintCallable)
    void SaveStashData(AController* Controller);

    // C++ Callback ���� ȣ���
    void SaveStashData(AController* Controller, FOnNetworkResponse Callback);

    // â�� �κ��丮 ���� BP
    UFUNCTION(BlueprintCallable)
    void LoadStashData(AController* Controller);

    void LoadStashData(AController* Controller, FOnNetworkResponse Callback);

    // �����ϰ� ������ �ε� 
    UFUNCTION(BlueprintCallable)
    void SaveLoadStashData(AController* Controller);

    UPROPERTY(BlueprintAssignable)
    FOnLoadUserProfile OnLoadUserProfile;

    UPROPERTY(BlueprintAssignable)
    FOnSellToSystem OnSellToSystem;

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

    UPROPERTY(BlueprintAssignable)
    FOnTradeResult OnTradeResult;

};
