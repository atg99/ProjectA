// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/MarketSubsystem.h"
#include "JsonUtilities.h"
#include "Utils/NetworkUtil.h"
#include "Kismet/GameplayStatics.h"
#include "Lobby/LobbyGameMode.h"
#include "GameFramework/PlayerState.h"
#include "ATGInventoryComponent.h"
#include "ATGContainerComponent.h"
#include "Utils/ATGSerializationLibrary.h"


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

    FString Endpoint = FString::Printf(TEXT("/api/v1/market/listings?page=%d&limit=%d&sort=%s&keyword=%s"), Page, Limit, *Sort, *Keyword);

    // NetworkSubsystem에게 통신 위임
    Network->SendRequest("GET", Endpoint, "", FOnNetworkResponse::CreateLambda(
        [this](bool bSuccess, FString ResponseContent)
        {
            if (!bSuccess)
            {
                FMarketMessageResponse Message;
                if (FJsonObjectConverter::JsonObjectStringToUStruct<FMarketMessageResponse>(ResponseContent, &Message, 0, 0))
                {
                    NET_LOG2(FString::Printf(TEXT("%s"), *Message.message));
                }
                return;
            }
            NET_LOG2(FString::Printf(TEXT("%s"), *ResponseContent));
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
    
    FString Status = "active";
    switch (StatusType)
    {
    case EListingStatusType::None:
        break;
    case EListingStatusType::Active:
        Status = "active";
        break;
    case EListingStatusType::Sold:
        Status = "sold";
        break;
    case EListingStatusType::History:
        Status = "history";
        break;
    default:
        break;
    }

    FString EndPoint = FString::Printf(TEXT("/api/v1/market/my-listings?status=%s"), *Status);

    Network->SendRequest("GET", EndPoint, "", FOnNetworkResponse::CreateLambda(
        [this](bool bSuccess, FString ResponseContent)
        {
            if (!bSuccess)
            {
                FMarketMessageResponse Message;
                if (FJsonObjectConverter::JsonObjectStringToUStruct<FMarketMessageResponse>(ResponseContent, &Message, 0, 0))
                {
                    NET_LOG2(FString::Printf(TEXT("%s"), *Message.message));
                }
                return;
            }
            NET_LOG2(FString::Printf(TEXT("%s"), *ResponseContent));
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

    FString EndPoint = FString::Printf(TEXT("/api/v1/market/listings/%d"), LisingID);

    Network->SendRequest("GET", EndPoint, "", FOnNetworkResponse::CreateLambda(
        [this](bool bSuccess, FString ResponseContent)
        {
            if (!bSuccess)
            {
                FMarketMessageResponse Message;
                if (FJsonObjectConverter::JsonObjectStringToUStruct<FMarketMessageResponse>(ResponseContent, &Message, 0, 0))
                {
                    NET_LOG2(FString::Printf(TEXT("%s"), *Message.message));
                }
                return;
            }
            NET_LOG2(FString::Printf(TEXT("%s"), *ResponseContent));
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

void UMarketSubsystem::RequestRegisterListing(int32 ItemDBID, APlayerController* PC, int32 Price, int32 Qty)
{
    
    // save stash 
    SaveStashData(PC, FOnNetworkResponse::CreateLambda(
        [this, PC, ItemDBID, Price, Qty](bool bSuccess, FString ResponseContent)
        {
            if (!bSuccess)
            {
                return;
            }

            UNetworkGameInstanceSubsystem* Network = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
            if (!Network) return;

            FString EndPoint = FString::Printf(TEXT("/api/v1/market/listings"));

            TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject);
            JsonObj->SetNumberField("item_entry_id", ItemDBID);
            JsonObj->SetNumberField("price", Price);
            JsonObj->SetNumberField("qty", Qty);

            FString JsonBody;
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonBody);
            FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);

            Network->SendRequest("POST", EndPoint, JsonBody, FOnNetworkResponse::CreateLambda(
                [this, PC](bool bSuccess, FString ResponseContent)
                {
                    if (!bSuccess)
                    {
                        FMarketMessageResponse Message;
                        if (FJsonObjectConverter::JsonObjectStringToUStruct<FMarketMessageResponse>(ResponseContent, &Message, 0, 0))
                        {
                            NET_LOG2(FString::Printf(TEXT("%s"), *Message.message));
                        }
                        return;
                    }
                    NET_LOG2(FString::Printf(TEXT("%s"), *ResponseContent));
                    FMarketMessageResponse Message;
                    if (FJsonObjectConverter::JsonObjectStringToUStruct<FMarketMessageResponse>(ResponseContent, &Message, 0, 0))
                    {
                        OnItemRegistered.Broadcast(Message);

                        // reload stash 
                        ALobbyGameMode* LobbyGM = Cast<ALobbyGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
                        if (LobbyGM)
                        {
                            LobbyGM->LoadStashData(PC);
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("JSON Parsing Failed: %s"), *ResponseContent);
                    }
                }
            ));
        }
    ));
    
}

void UMarketSubsystem::RequestPurchaseListing(int32 LisingID)
{
    UNetworkGameInstanceSubsystem* Network = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!Network) return;

    FString EndPoint = FString::Printf(TEXT("/api/v1/market/listings/%d/purchase"), LisingID);

    Network->SendRequest("POST", EndPoint, "", FOnNetworkResponse::CreateLambda(
        [this](bool bSuccess, FString ResponseContent)
        {
            if (!bSuccess)
            {
                FMarketMessageResponse Message;
                if (FJsonObjectConverter::JsonObjectStringToUStruct<FMarketMessageResponse>(ResponseContent, &Message, 0, 0))
                {
                    NET_LOG2(FString::Printf(TEXT("%s"), *Message.message));
                }
                return;
            }
            NET_LOG2(FString::Printf(TEXT("%s"), *ResponseContent));
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

    FString EndPoint = FString::Printf(TEXT("/api/v1/market/listings/%d/cancel"), LisingID);

    Network->SendRequest("POST", EndPoint, "", FOnNetworkResponse::CreateLambda(
        [this](bool bSuccess, FString ResponseContent)
        {
            if (!bSuccess)
            {
                FMarketMessageResponse Message;
                if (FJsonObjectConverter::JsonObjectStringToUStruct<FMarketMessageResponse>(ResponseContent, &Message, 0, 0))
                {
                    NET_LOG2(FString::Printf(TEXT("%s"), *Message.message));
                }
                return;
            }
            NET_LOG2(FString::Printf(TEXT("%s"), *ResponseContent));
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

void UMarketSubsystem::SaveStashData(AController* Controller, FOnNetworkResponse Callback)
{
    UNetworkGameInstanceSubsystem* Network = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!Network)
    {
        Callback.ExecuteIfBound(false, TEXT("No UNetworkGameInstanceSubsystem"));
        return;
    }

    APlayerState* PlayerState = Controller->GetPlayerState<APlayerState>();
    if (PlayerState)
    {
        UActorComponent* AC = PlayerState->GetComponentByClass(UATGContainerComponent::StaticClass());
        if (AC)
        {
            UATGContainerComponent* StashComp = Cast<UATGContainerComponent>(AC);
            if (StashComp)
            {
                FString GridJson = UATGSerializationLibrary::ConvertGridToJson(StashComp->GetContainerInventory());

                FString EndPoint = FString::Printf(TEXT("/api/v1/stash/save"));

                Network->SendRequest("POST", EndPoint, GridJson, FOnNetworkResponse::CreateLambda(
                    [this, Callback](bool bSuccess, FString ResponseContent)
                    {
                        FBackendSaveStashResult Message;
                        if (FJsonObjectConverter::JsonObjectStringToUStruct<FBackendSaveStashResult>(ResponseContent, &Message, 0, 0))
                        {
                            NET_LOG2(FString::Printf(TEXT(" %d %s"), bSuccess, *Message.message));
                        }

                        Callback.ExecuteIfBound(bSuccess, ResponseContent);

                        if (!bSuccess)
                        {
                            return;
                        }
                    }
                ));
                return;
            }
        }
    }
    NET_LOG2(TEXT("Missing Component or PlayerState"));
    Callback.ExecuteIfBound(false, TEXT("Inventory Component Not Found"));
}