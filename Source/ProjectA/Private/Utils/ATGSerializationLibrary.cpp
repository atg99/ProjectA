// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/ATGSerializationLibrary.h"
#include "JsonUtilities.h"
#include "Data/ATGItemData.h"
#include "Engine/AssetManager.h"
#include "ATGInventoryOwnerInterface.h"

TSharedPtr<FJsonObject> UATGSerializationLibrary::SerializeActorToJson(AActor* Actor)
{
	if (!Actor) return nullptr;

	TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject);

	// 1. Ŭ���� ���� ���� (�ε��� �� �����ϱ� ����)
	JsonObj->SetStringField(TEXT("ClassPath"), Actor->GetClass()->GetPathName());

	// 2. Transform ���� (��ġ/ȸ��/������) -> �⺻������ �����ϴ� ���� ����;
	// FJsonObjectConverter�� �̿��� FTransform ����ü�� �ٷ� JSON ��ü�� ��ȯ

	FTransform ActorTransform = Actor->GetActorTransform();
	//FJsonObjectConverter::UStructToJsonObjectString;
	//TSharedPtr<FJsonObject> TransformJson = FJsonObjectConverter::UStructToJsonObject(ActorTransform);
	TSharedRef<FJsonObject> TransformJson = MakeShared<FJsonObject>();

	FJsonObjectConverter::UStructToJsonObject(TBaseStructure<FTransform>::Get(), &ActorTransform, TransformJson);

	JsonObj->SetObjectField(TEXT("Transform"), TransformJson);

	// 3. ���÷���: Actor�� ��� ������Ƽ ��ȸ
	for (TFieldIterator<FProperty> PropIt(Actor->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;

		// 'SaveGame' �±װ� ������ �ǳʶ�
		if (!Property->HasAnyPropertyFlags(CPF_SaveGame))
		{
			continue;
		}

		FString PropName = Property->GetName();

		// --- �ڷ����� ó�� ---

		// 3-1. ���� (Int, Float, Byte ��)
		if (FNumericProperty* NumProp = CastField<FNumericProperty>(Property))
		{
			// �������� �Ǽ����� ����
			if (NumProp->IsFloatingPoint())
			{
				float Val = NumProp->GetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<void>(Actor));
				JsonObj->SetNumberField(PropName, Val);
			}
			else if (NumProp->IsInteger())
			{
				int64 Val = NumProp->GetSignedIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Actor));
				JsonObj->SetNumberField(PropName, Val);
			}
		}
		// 3-2. �Ҹ��� (Bool)
		else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
		{
			bool Val = BoolProp->GetPropertyValue_InContainer(Actor);
			JsonObj->SetBoolField(PropName, Val);
		}
		// 3-3. ���ڿ� (String)
		else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
		{
			FString Val = StrProp->GetPropertyValue_InContainer(Actor);
			JsonObj->SetStringField(PropName, Val);
		}
		// 3-4. ����ü (Vector, Rotator, Custom Struct ��)
		else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
		{
			// ����ü ���� �����͸� JSON Object�� ��ȯ
			const void* StructAddr = StructProp->ContainerPtrToValuePtr<void>(Actor);
			TSharedRef<FJsonObject> StructJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(StructProp->Struct, StructAddr, StructJson);
			// �𸮾� ���� ������ ��� (�ſ� ������)
			JsonObj->SetObjectField(PropName, StructJson);
		}
	}

	return JsonObj;
}

void UATGSerializationLibrary::DeserializeJsonToActor(AActor* Actor, TSharedPtr<FJsonObject> JsonObj)
{
	if (!Actor || !JsonObj.IsValid()) return;

	// 1. Transform�� �̹� Spawn�� �� ���� �״� �����ϰų�, ���⼭ ���� ������Ʈ ����
	// ...

	// 2. ���÷���: Actor�� ��� ������Ƽ ��ȸ
	for (TFieldIterator<FProperty> PropIt(Actor->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		FString PropName = Property->GetName();

		// JSON�� �ش� Ű���� ���ų�, SaveGame �±װ� ������ �н�
		if (!JsonObj->HasField(PropName) || !Property->HasAnyPropertyFlags(CPF_SaveGame))
		{
			continue;
		}

		// --- �ڷ����� �� ���� ---

		// 2-1. ���� (Int, Float)
		if (FNumericProperty* NumProp = CastField<FNumericProperty>(Property))
		{
			double Val = JsonObj->GetNumberField(PropName);

			if (NumProp->IsFloatingPoint())
			{
				NumProp->SetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<void>(Actor), Val);
			}
			else if (NumProp->IsInteger())
			{
				NumProp->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Actor), (int64)Val);
			}
		}
		// 2-2. �Ҹ��� (Bool)
		else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
		{
			bool Val = JsonObj->GetBoolField(PropName);
			BoolProp->SetPropertyValue_InContainer(Actor, Val);
		}
		// 2-3. ���ڿ� (String)
		else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
		{
			FString Val = JsonObj->GetStringField(PropName);
			StrProp->SetPropertyValue_InContainer(Actor, Val);
		}
		// 2-4. ����ü (Vector ��)
		else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
		{
			const TSharedPtr<FJsonObject>* ChildObj;
			if (JsonObj->TryGetObjectField(PropName, ChildObj))
			{
				void* StructAddr = StructProp->ContainerPtrToValuePtr<void>(Actor);
				// JSON Object -> UStruct �޸𸮷� �ٷ� ����
				FJsonObjectConverter::JsonObjectToUStruct(ChildObj->ToSharedRef(), StructProp->Struct, StructAddr, 0, 0);
			}
		}
	}
}

FString UATGSerializationLibrary::ConvertGridToJson(const FInventoryGrid& Grid)
{

	FInventorySaveData SaveData;

	// Grid �⺻ ���� ����
	SaveData.grid_width = Grid.GridWidth;
	SaveData.grid_height = Grid.GridHeight;

	// Entries ��ȸ�ϸ� DTO�� ��ȯ
	for (const FInventoryEntry& Entry : Grid.Entries)
	{
		// ������ ���ų� �������� ��ȿ���� ������ ��ŵ
		if (Entry.Quantity <= 0 || Entry.Item.IsNull())
		{
			continue;
		}

		FInventoryEntrySaveData EntryData;

		UATGItemData* ItemRaw = Entry.Item.Get();
		if (!ItemRaw)
		{
			ItemRaw = Entry.Item.LoadSynchronous();

		}

		if (ItemRaw)
		{
			// PrimaryAssetId
			EntryData.primary_asset_id = ItemRaw->GetPrimaryAssetId().ToString();
		}
		else
		{
			continue;
		}

		EntryData.qty = Entry.Quantity;
		EntryData.x = Entry.X;
		EntryData.y = Entry.Y;
		EntryData.b_rotated = Entry.bRotated;

		EntryData.item_entry_id = Entry.DBId;

		SaveData.saved_entries.Add(EntryData);
	}

	// Struct -> JSON String
	FString JsonString;
	//FJsonObjectConverter::UStructToJsonObject<FInventoryEntrySaveData>(SaveData,)
	FJsonObjectConverter::UStructToJsonObjectString(SaveData, JsonString);

	return JsonString;
}

bool UATGSerializationLibrary::ConvertDataToGrid(const FInventorySaveData& LoadedData, FInventoryGrid& OutGrid)
{
	OutGrid.Entries.Empty();
	OutGrid.MarkArrayDirty();

	UAssetManager& AssetManager = UAssetManager::Get();

	// DTO -> Runtime Entry ��ȯ
	for (const FInventoryEntrySaveData& SavedEntry : LoadedData.saved_entries)
	{
		// �ʱ� Null üũ ������ (���⼭ üũ�ϸ� �ȵ�)
		FInventoryEntry NewEntry;

		// Asset ID Ȯ��
		FPrimaryAssetId AssetId = FPrimaryAssetId::FromString(SavedEntry.primary_asset_id);
		if (!AssetId.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("Invalid PrimaryAssetId String: %s"), *SavedEntry.primary_asset_id);
			continue;
		}

		// Asset Path ã��
		FSoftObjectPath ItemPath = AssetManager.GetPrimaryAssetPath(AssetId);
		if (!ItemPath.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("Asset Manager could not find path for ID: %s"), *SavedEntry.primary_asset_id);
			continue;
		}

		// Soft Pointer �Ҵ�
		NewEntry.Item = TSoftObjectPtr<UATGItemData>(ItemPath);

		NewEntry.Quantity = SavedEntry.qty;
		NewEntry.X = SavedEntry.x;
		NewEntry.Y = SavedEntry.y;
		NewEntry.bRotated = SavedEntry.b_rotated;

		// DB Item PK ���� �ŷ��ҿ� ������ �Ȱų� �Ҷ� �ʿ�
		SavedEntry.item_entry_id;

		UATGItemData* ItemData = NewEntry.Item.LoadSynchronous();
		if (ItemData)
		{
			NewEntry.Width = ItemData->Width;
			NewEntry.Height = ItemData->Height;

			OutGrid.AddItemAt(
				NewEntry.Item,
				NewEntry.Quantity,
				NewEntry.X,
				NewEntry.Y,
				ItemData->Width,
				ItemData->Height,
				NewEntry.bRotated,
				-1,
				SavedEntry.item_entry_id
			);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ConvertJsonToGrid : Load Item Fail !!! Asset: %s"), *SavedEntry.primary_asset_id);
			continue;
		}
	}

	OutGrid.MarkArrayDirty();

	int32 GridWidth = LoadedData.grid_width;
	int32 GridHeight = LoadedData.grid_height;

	OutGrid.Owner->SetGridSize(GridWidth, GridHeight);

	return true;
}

UPrimaryDataAsset* UATGSerializationLibrary::GetPrimaryAssetfromPrimaryAssetName(const FString& PrimaryAssetName)
{
	UAssetManager& AssetManager = UAssetManager::Get();

	FPrimaryAssetId AssetId = FPrimaryAssetId::FromString(PrimaryAssetName);
	if (!AssetId.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid PrimaryAssetId String: %s"), *PrimaryAssetName);
		return nullptr;
	}

	// Asset Path ã��
	FSoftObjectPath ItemPath = AssetManager.GetPrimaryAssetPath(AssetId);
	if (!ItemPath.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Asset Manager could not find path for ID: %s"), *PrimaryAssetName);
		return nullptr;
	}

	// Soft Pointer �Ҵ�
	TSoftObjectPtr<UPrimaryDataAsset> Item = TSoftObjectPtr<UPrimaryDataAsset>(ItemPath);
	if (UPrimaryDataAsset* ItemData = Item.Get())
	{
		return ItemData;
	}
	else
	{
		UPrimaryDataAsset* ItemDat = Item.LoadSynchronous();
		return ItemDat;
	}
}