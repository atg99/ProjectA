// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/MarketSubsystem.h"
#include "Title/NetworkGameInstanceSubsystem.h"
#include "JsonUtilities.h"

void UMarketSubsystem::RequestMarketListings(int32 Page, int32 Limit, EMarketSortType SortType, FString Keyword)
{
    UNetworkGameInstanceSubsystem* Network = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!Network) return;

    FString Sort = "created_at_desc";
    switch (SortType)
    {
    case EMarketSortType::None:
        break;
    case EMarketSortType::PriceASC:
        Sort = "price_asc";
        break;
    case EMarketSortType::PriceDESC:
        Sort = "price_desc";
        break;
    case EMarketSortType::CreatedAtDESC:
        Sort = "created_at_desc";
        break;
    default:
        break;
    }

    FString Endpoint = FString::Printf(TEXT("/market/listings?page=%d?limit=%d?sort=%s?keyword=%s"), Page, Limit, *Sort, *Keyword);

    // NetworkSubsystem에게 통신 위임
    Network->SendRequest("GET", Endpoint, "", FOnNetworkResponse::CreateLambda(
        [this](bool bSuccess, FString ResponseContent)
        {
            if (!bSuccess) return;

            FMarketListingsResponse ResponseData;
            if (FJsonObjectConverter::JsonObjectStringToUStruct<FMarketListingsResponse>(ResponseContent, &ResponseData, 0, 0))
            {
                OnMarketItemsUpdated.Broadcast(ResponseData.data, ResponseData.pagination);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("JSON Parsing Failed: %s"), *ResponseContent);
            }
        }
    ));
}

void UMarketSubsystem::RequestMyListings(EListingStatusType StatusType)
{
    UNetworkGameInstanceSubsystem* Network = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!Network) return;
    
    FString Status = "Active";
    switch (StatusType)
    {
    case EListingStatusType::None:
        break;
    case EListingStatusType::Active:
        Status = "Active";
        break;
    case EListingStatusType::Sold:
        Status = "Sold";
        break;
    case EListingStatusType::History:
        Status = "History";
        break;
    default:
        break;
    }

    FString EndPoint = FString::Printf(TEXT("/market/my-listings?status=%s"), *Status);

    Network->SendRequest("GET", EndPoint, "", FOnNetworkResponse::CreateLambda(
        [this](bool bSuccess, FString ResponseContent)
        {
            if (!bSuccess) return;

            TArray<FMarketListingItem> MyItems;
            if (FJsonObjectConverter::JsonArrayStringToUStruct<FMarketListingItem>(ResponseContent, &MyItems, 0, 0))
            {
                OnMyItemsUpdated.Broadcast(MyItems);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("JSON Parsing Failed: %s"), *ResponseContent);
            }
        }
    ));
}

void UMarketSubsystem::RequestLookupListing(int32 LisingID)
{
    UNetworkGameInstanceSubsystem* Network = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!Network) return;

    FString EndPoint = FString::Printf(TEXT("/listings/:%d"), LisingID);

    Network->SendRequest("GET", EndPoint, "", FOnNetworkResponse::CreateLambda(
        [this](bool bSuccess, FString ResponseContent)
        {
            if (!bSuccess) return;

            FMarketListingItem LookupItem;
            if (FJsonObjectConverter::JsonObjectStringToUStruct<FMarketListingItem>(ResponseContent, &LookupItem, 0, 0))
            {
                OnLookupItem.Broadcast(LookupItem);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("JSON Parsing Failed: %s"), *ResponseContent);
            }
        }
    ));
}

void UMarketSubsystem::RequestRegisterListing(int32 ItemDBID, int32 Price, int32 Qty)
{
    UNetworkGameInstanceSubsystem* Network = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!Network) return;

    FString EndPoint = FString::Printf(TEXT("/listings?item_entry_id=%d?price=%d?qty=%d"), ItemDBID, Price, Qty);

    Network->SendRequest("POST", EndPoint, "", FOnNetworkResponse::CreateLambda(
        [this](bool bSuccess, FString ResponseContent)
        {
            if (!bSuccess) return;

            FMarketMessageResponse Message;
            if (FJsonObjectConverter::JsonObjectStringToUStruct<FMarketMessageResponse>(ResponseContent, &Message, 0, 0))
            {
                OnItemRegistered.Broadcast(Message);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("JSON Parsing Failed: %s"), *ResponseContent);
            }
        }
    ));
}

void UMarketSubsystem::RequestPurchaseListing(int32 LisingID)
{
    UNetworkGameInstanceSubsystem* Network = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!Network) return;

    FString EndPoint = FString::Printf(TEXT("/listings/:%d/purchase"), LisingID);

    Network->SendRequest("POST", EndPoint, "", FOnNetworkResponse::CreateLambda(
        [this](bool bSuccess, FString ResponseContent)
        {
            if (!bSuccess) return;

            FMarketMessageResponse Message;
            if (FJsonObjectConverter::JsonObjectStringToUStruct<FMarketMessageResponse>(ResponseContent, &Message, 0, 0))
            {
                OnItemPurchased.Broadcast(Message);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("JSON Parsing Failed: %s"), *ResponseContent);
            }
        }
    ));
}

void UMarketSubsystem::RequestCancelListing(int32 LisingID)
{
    UNetworkGameInstanceSubsystem* Network = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!Network) return;

    FString EndPoint = FString::Printf(TEXT("/listings/:%d/cancel"), LisingID);

    Network->SendRequest("POST", EndPoint, "", FOnNetworkResponse::CreateLambda(
        [this](bool bSuccess, FString ResponseContent)
        {
            if (!bSuccess) return;

            FMarketMessageResponse Message;
            if (FJsonObjectConverter::JsonObjectStringToUStruct<FMarketMessageResponse>(ResponseContent, &Message, 0, 0))
            {
                OnItemCanceled.Broadcast(Message);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("JSON Parsing Failed: %s"), *ResponseContent);
            }
        }
    ));
}


