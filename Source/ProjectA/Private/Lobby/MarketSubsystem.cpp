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


void UMarketSubsystem::RequestUserProfile()
{
    UNetworkGameInstanceSubsystem* Network = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!Network) return;

    FString Endpoint = FString::Printf(TEXT("/api/v1/auth/profile"));

    Network->SendRequest("GET", Endpoint, "", FOnNetworkResponse::CreateLambda(
        [this](bool bSuccess, FString ResponseContent, int32 Code)
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
            FUserProfile ResponseData;
            if (FJsonObjectConverter::JsonObjectStringToUStruct<FUserProfile>(ResponseContent, &ResponseData, 0, 0))
            {
                OnLoadUserProfile.Broadcast(ResponseData, Code);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("JSON Parsing Failed: %s"), *ResponseContent);
            }
        }
    ));
}

void UMarketSubsystem::RequestSellToSystem(EStorageSourceType SourceType, int32 ItemDBID, int32 Qty)
{
    UNetworkGameInstanceSubsystem* Network = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!Network) return;

    FString SType = "";
    switch (SourceType)
    {
    case EStorageSourceType::None:
        return;
    case EStorageSourceType::Inventory:
        SType = "inventory";
        break;
    case EStorageSourceType::Stash:
        SType = "stash";
        break;
    default:
        return;
    }

    FString Endpoint = FString::Printf(TEXT("/api/v1/shop/sell?source_type=%s&item_entry_id=%d&qty=%d"), *SType, ItemDBID, Qty);

    Network->SendRequest("POST", Endpoint, "", FOnNetworkResponse::CreateLambda(
        [this](bool bSuccess, FString ResponseContent, int32 Code)
        {
            if (!bSuccess)
            {
                FMarketMessageResponse Message;
                if (FJsonObjectConverter::JsonObjectStringToUStruct<FMarketMessageResponse>(ResponseContent, &Message, 0, 0))
                {
                    NET_LOG2(FString::Printf(TEXT("%s"), *Message.message));
                    FSellToSystemResult Result;
                    Result.message = Message.message;
                    OnSellToSystem.Broadcast(Result, Code);
                }
                return;
            }

            FSellToSystemResult SellToSystemResult;
            if (FJsonObjectConverter::JsonObjectStringToUStruct<FSellToSystemResult>(ResponseContent, &SellToSystemResult, 0, 0))
            {
                OnSellToSystem.Broadcast(SellToSystemResult, Code);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("JSON Parsing Failed: %s"), *ResponseContent);
            }
        }
    ));
}

void UMarketSubsystem::RequestTradeItems(const FTradeItemRequest& TradeItemRequest)
{
    UNetworkGameInstanceSubsystem* Network = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!Network) return;

    FString SType = "stash";

    FString Endpoint = FString::Printf(TEXT("/api/v1/shop/trade"));

    FString JsonBody;
    if (!FJsonObjectConverter::UStructToJsonObjectString(TradeItemRequest, JsonBody))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to serialize Trade Request"));
        return;
    }

    NET_LOG2(FString::Printf(TEXT("%s"), *JsonBody));

    Network->SendRequest("POST", Endpoint, JsonBody, FOnNetworkResponse::CreateLambda(
        [this](bool bSuccess, FString ResponseContent, int32 Code)
        {
            if (!bSuccess)
            {
                FMarketMessageResponse Message;
                if (FJsonObjectConverter::JsonObjectStringToUStruct<FMarketMessageResponse>(ResponseContent, &Message, 0, 0))
                {
                    NET_LOG2(FString::Printf(TEXT("%s"), *Message.message));
                    FTradeResult Result;
                    Result.message = Message.message;
                    OnTradeResult.Broadcast(Result, Code);
                }
                return;
            }

            FTradeResult TradeResult;
            if (FJsonObjectConverter::JsonObjectStringToUStruct<FTradeResult>(ResponseContent, &TradeResult, 0, 0))
            {
                OnTradeResult.Broadcast(TradeResult, Code);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("JSON Parsing Failed: %s"), *ResponseContent);
            }
        }
    ));
}

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
        [this](bool bSuccess, FString ResponseContent, int32 Code)
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

void UMarketSubsystem::RequestMyListings(EListingStatusType StatusType, int32 Page, int32 Limit, FString Keyword)
{
    UNetworkGameInstanceSubsystem* Network = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!Network) return;
    
    FString Status = "";
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

    FString EndPoint = FString::Printf(TEXT("/api/v1/market/my-listings?status=%s&page=%d&limit=%d&keyword=%s"), *Status, Page, Limit, *Keyword);

    Network->SendRequest("GET", EndPoint, "", FOnNetworkResponse::CreateLambda(
        [this](bool bSuccess, FString ResponseContent, int32 Code)
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
            FMarketListingsResponse MyListings;
            if (FJsonObjectConverter::JsonObjectStringToUStruct<FMarketListingsResponse>(ResponseContent, &MyListings, 0, 0))
            {
                OnMyItemsUpdated.Broadcast(MyListings.data, MyListings.pagination);
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
        [this](bool bSuccess, FString ResponseContent, int32 Code)
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
        [this, PC, ItemDBID, Price, Qty](bool bSuccess, FString ResponseContent, int32 Code)
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
                [this, PC](bool bSuccess, FString ResponseContent, int32 Code)
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
                        OnItemRegistered.Broadcast(Message, Code);

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
        [this](bool bSuccess, FString ResponseContent, int32 Code)
        {
            if (!bSuccess)
            {
                FMarketMessageResponse Message;
                if (FJsonObjectConverter::JsonObjectStringToUStruct<FMarketMessageResponse>(ResponseContent, &Message, 0, 0))
                {
                    NET_LOG2(FString::Printf(TEXT("%s"), *Message.message));
                    OnItemPurchased.Broadcast(Message, Code);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("JSON Parsing Failed: %s"), *ResponseContent);
                }
                return;
            }
            NET_LOG2(FString::Printf(TEXT("%s"), *ResponseContent));
            FMarketMessageResponse Message;
            if (FJsonObjectConverter::JsonObjectStringToUStruct<FMarketMessageResponse>(ResponseContent, &Message, 0, 0))
            {
                OnItemPurchased.Broadcast(Message, Code);
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
        [this](bool bSuccess, FString ResponseContent, int32 Code)
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
                OnItemCanceled.Broadcast(Message, Code);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("JSON Parsing Failed: %s"), *ResponseContent);
            }
        }
    ));
}

void UMarketSubsystem::SaveStashData(AController* Controller)
{
    FBackendSaveStashResult SaveStashResult;
    UNetworkGameInstanceSubsystem* Network = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!Network)
    {
        OnStashSaveResult.Broadcast(SaveStashResult, -1);
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
                    [this](bool bSuccess, FString ResponseContent, int32 Code)
                    {
                        FBackendSaveStashResult SaveStashResult;

                        if (FJsonObjectConverter::JsonObjectStringToUStruct<FBackendSaveStashResult>(ResponseContent, &SaveStashResult, 0, 0))
                        {
                            NET_LOG2(FString::Printf(TEXT(" %d %s"), bSuccess, *SaveStashResult.message));
                        }

                        OnStashSaveResult.Broadcast(SaveStashResult, Code);

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
    OnStashSaveResult.Broadcast(SaveStashResult, -1);
}

void UMarketSubsystem::SaveStashData(AController* Controller, FOnNetworkResponse Callback)
{
    UNetworkGameInstanceSubsystem* Network = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!Network)
    {
        Callback.ExecuteIfBound(false, TEXT("No UNetworkGameInstanceSubsystem"), -1);
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
                    [this, Callback](bool bSuccess, FString ResponseContent, int32 Code)
                    {
                        FBackendSaveStashResult Message;
                        if (FJsonObjectConverter::JsonObjectStringToUStruct<FBackendSaveStashResult>(ResponseContent, &Message, 0, 0))
                        {
                            NET_LOG2(FString::Printf(TEXT(" %d %s"), bSuccess, *Message.message));
                        }

                        Callback.ExecuteIfBound(bSuccess, ResponseContent, Code);

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
    Callback.ExecuteIfBound(false, TEXT("Inventory Component Not Found"), -1);
}

void UMarketSubsystem::LoadStashData(AController* Controller)
{
    UNetworkGameInstanceSubsystem* Network = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!Network)
    {
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

                FString EndPoint = FString::Printf(TEXT("/api/v1/stash/load"));

                Network->SendRequest("POST", EndPoint, GridJson, FOnNetworkResponse::CreateLambda(
                    [this, StashComp](bool bSuccess, FString ResponseContent, int32 Code)
                    {
                        FInventorySaveData LoadData;

                        if (FJsonObjectConverter::JsonObjectStringToUStruct<FInventorySaveData>(ResponseContent, &LoadData, 0, 0))
                        {
                            NET_LOG2(FString::Printf(TEXT(" %d %d"), bSuccess, Code));
                        }

                        if (!bSuccess)
                        {
                            return;
                        }

                        NET_LOG(FString::Printf(TEXT("Code : %d, entry num : %d"), Code, LoadData.saved_entries.Num()));
                        for (const FInventoryEntrySaveData& entry : LoadData.saved_entries)
                        {
                            NET_LOG(FString::Printf(TEXT("asset id : %s, DBId : %d"), *entry.primary_asset_id, entry.item_entry_id));
                        }
                        if (IsValid(StashComp))
                        {
                            UATGSerializationLibrary::ConvertDataToGrid(LoadData, StashComp->GetContainerInventory());
                            //StashComp->OnRebuildAll.Broadcast(-1);
                        }
                    }
                ));
                return;
            }
        }
    }
    NET_LOG2(TEXT("Missing Component or PlayerState"));
}

void UMarketSubsystem::LoadStashData(AController* Controller, FOnNetworkResponse Callback)
{
    UNetworkGameInstanceSubsystem* Network = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!Network)
    {
        Callback.ExecuteIfBound(false, TEXT("Can't Find UNetworkGameInstanceSubsystem"), -1);
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

                FString EndPoint = FString::Printf(TEXT("/api/v1/stash/load"));

                Network->SendRequest("POST", EndPoint, GridJson, FOnNetworkResponse::CreateLambda(
                    [this, StashComp, Callback](bool bSuccess, FString ResponseContent, int32 Code)
                    {
                        FInventorySaveData LoadData;

                        if (FJsonObjectConverter::JsonObjectStringToUStruct<FInventorySaveData>(ResponseContent, &LoadData, 0, 0))
                        {
                            NET_LOG2(FString::Printf(TEXT(" %d %d"), bSuccess, Code));
                        }

                        if (!bSuccess)
                        {
                            Callback.ExecuteIfBound(false, ResponseContent, Code);
                            return;
                        }

                        NET_LOG(FString::Printf(TEXT("Code : %d, entry num : %d"), Code, LoadData.saved_entries.Num()));
                        for (const FInventoryEntrySaveData& entry : LoadData.saved_entries)
                        {
                            NET_LOG(FString::Printf(TEXT("asset id : %s, DBId : %d"), *entry.primary_asset_id, entry.item_entry_id));
                        }
                        if (IsValid(StashComp))
                        {
                            UATGSerializationLibrary::ConvertDataToGrid(LoadData, StashComp->GetContainerInventory());
                            //StashComp->OnRebuildAll.Broadcast(-1);
                            Callback.ExecuteIfBound(true, TEXT("Successful"), Code);
                        }
                        else
                        {
                            Callback.ExecuteIfBound(true, TEXT("StashComp Not Valid"), Code);
                        }
                    }
                ));
                return;
            }
        }
    }
    NET_LOG2(TEXT("Missing Component or PlayerState"));
    Callback.ExecuteIfBound(false, TEXT("Missing Component or PlayerState"), -1);
}

void UMarketSubsystem::SaveLoadStashData(AController* Controller)
{
    SaveStashData(Controller, FOnNetworkResponse::CreateLambda(
        [this, Controller](bool bSuccess, FString ResponseContent, int32 Code)
        {
            if (!bSuccess)
            {
                OnStashSaveAndLoad.Broadcast(false);
                return;
            }

            LoadStashData(Controller, FOnNetworkResponse::CreateLambda(
                [this](bool bSuccess, FString ResponseContent, int32 Code)
                {
                    OnStashSaveAndLoad.Broadcast(bSuccess);
                }
            ));
        }
    ));
}
