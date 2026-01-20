// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/ATGSerializationLibrary.h"
#include "JsonUtilities.h"
#include "Data/ATGItemData.h"

TSharedPtr<FJsonObject> UATGSerializationLibrary::SerializeActorToJson(AActor* Actor)
{
	if (!Actor) return nullptr;

	TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject);

	// 1. 클래스 정보 저장 (로드할 때 스폰하기 위함)
	JsonObj->SetStringField(TEXT("ClassPath"), Actor->GetClass()->GetPathName());

	// 2. Transform 저장 (위치/회전/스케일) -> 기본적으로 저장하는 것이 좋음;
	// FJsonObjectConverter를 이용해 FTransform 구조체를 바로 JSON 객체로 변환

	FTransform ActorTransform = Actor->GetActorTransform();
	//FJsonObjectConverter::UStructToJsonObjectString;
	//TSharedPtr<FJsonObject> TransformJson = FJsonObjectConverter::UStructToJsonObject(ActorTransform);
	TSharedRef<FJsonObject> TransformJson = MakeShared<FJsonObject>();

	FJsonObjectConverter::UStructToJsonObject(TBaseStructure<FTransform>::Get(), &ActorTransform, TransformJson);

	JsonObj->SetObjectField(TEXT("Transform"), TransformJson);

	// 3. 리플렉션: Actor의 모든 프로퍼티 순회
	for (TFieldIterator<FProperty> PropIt(Actor->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;

		// 'SaveGame' 태그가 없으면 건너뜀
		if (!Property->HasAnyPropertyFlags(CPF_SaveGame))
		{
			continue;
		}

		FString PropName = Property->GetName();

		// --- 자료형별 처리 ---

		// 3-1. 숫자 (Int, Float, Byte 등)
		if (FNumericProperty* NumProp = CastField<FNumericProperty>(Property))
		{
			// 정수인지 실수인지 구분
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
		// 3-2. 불리언 (Bool)
		else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
		{
			bool Val = BoolProp->GetPropertyValue_InContainer(Actor);
			JsonObj->SetBoolField(PropName, Val);
		}
		// 3-3. 문자열 (String)
		else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
		{
			FString Val = StrProp->GetPropertyValue_InContainer(Actor);
			JsonObj->SetStringField(PropName, Val);
		}
		// 3-4. 구조체 (Vector, Rotator, Custom Struct 등)
		else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
		{
			// 구조체 내부 데이터를 JSON Object로 변환
			const void* StructAddr = StructProp->ContainerPtrToValuePtr<void>(Actor);
			TSharedRef<FJsonObject> StructJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(StructProp->Struct, StructAddr, StructJson);
			// 언리얼 내장 컨버터 사용 (매우 편리함)
			JsonObj->SetObjectField(PropName, StructJson);
		}
	}

	return JsonObj;
}

void UATGSerializationLibrary::DeserializeJsonToActor(AActor* Actor, TSharedPtr<FJsonObject> JsonObj)
{
	if (!Actor || !JsonObj.IsValid()) return;

	// 1. Transform은 이미 Spawn할 때 썼을 테니 생략하거나, 여기서 강제 업데이트 가능
	// ...

	// 2. 리플렉션: Actor의 모든 프로퍼티 순회
	for (TFieldIterator<FProperty> PropIt(Actor->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		FString PropName = Property->GetName();

		// JSON에 해당 키값이 없거나, SaveGame 태그가 없으면 패스
		if (!JsonObj->HasField(PropName) || !Property->HasAnyPropertyFlags(CPF_SaveGame))
		{
			continue;
		}

		// --- 자료형별 값 주입 ---

		// 2-1. 숫자 (Int, Float)
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
		// 2-2. 불리언 (Bool)
		else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
		{
			bool Val = JsonObj->GetBoolField(PropName);
			BoolProp->SetPropertyValue_InContainer(Actor, Val);
		}
		// 2-3. 문자열 (String)
		else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
		{
			FString Val = JsonObj->GetStringField(PropName);
			StrProp->SetPropertyValue_InContainer(Actor, Val);
		}
		// 2-4. 구조체 (Vector 등)
		else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
		{
			const TSharedPtr<FJsonObject>* ChildObj;
			if (JsonObj->TryGetObjectField(PropName, ChildObj))
			{
				void* StructAddr = StructProp->ContainerPtrToValuePtr<void>(Actor);
				// JSON Object -> UStruct 메모리로 바로 복사
				FJsonObjectConverter::JsonObjectToUStruct(ChildObj->ToSharedRef(), StructProp->Struct, StructAddr, 0, 0);
			}
		}
	}
}

FString UATGSerializationLibrary::ConvertGridToJson(const FInventoryGrid& Grid)
{

	FInventorySaveData SaveData;

	// Grid 기본 정보 저장
	SaveData.GridWidth = Grid.GridWidth;
	SaveData.GridHeight = Grid.GridHeight;

	// Entries 순회하며 DTO로 변환
	for (const FInventoryEntry& Entry : Grid.Entries)
	{
		// 수량이 없거나 아이템이 유효하지 않으면 스킵 (선택 사항)
		if (Entry.Quantity <= 0 || Entry.Item.IsNull())
		{
			continue;
		}

		FInventoryEntrySaveData EntryData;
		// TSoftObjectPtr -> String 변환
		EntryData.ItemAssetPath = Entry.Item.ToSoftObjectPath().ToString();

		EntryData.Quantity = Entry.Quantity;
		EntryData.X = Entry.X;
		EntryData.Y = Entry.Y;
		EntryData.bRotated = Entry.bRotated;
		//EntryData.Id = Entry.Id; // 필요하다면 저장

		SaveData.SavedEntries.Add(EntryData);
	}

	// 3. Struct -> JSON String 변환
	FString JsonString;
	FJsonObjectConverter::UStructToJsonObjectString(SaveData, JsonString);

	return JsonString;
}

bool UATGSerializationLibrary::ConvertJsonToGrid(const FString& JsonString, FInventoryGrid& OutGrid)
{
	if (JsonString.IsEmpty()) return false;

	FInventorySaveData LoadedData;

	// 1. JSON String -> Struct 변환
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &LoadedData))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse Inventory JSON"));
		return false;
	}

	// 기존 인벤토리 클리어
	OutGrid.Entries.Empty();
	OutGrid.MarkArrayDirty();

	OutGrid.GridWidth = LoadedData.GridWidth;
	OutGrid.GridHeight = LoadedData.GridHeight;

	// DTO -> Runtime Entry 변환
	for (const FInventoryEntrySaveData& SavedEntry : LoadedData.SavedEntries)
	{
		FInventoryEntry NewEntry;
		if (NewEntry.Item.IsNull())
		{
			UE_LOG(LogTemp, Error, TEXT("ConvertJsonToGrid : Item IsNull !!!"));
			continue;
		}
		// String -> TSoftObjectPtr 변환
		NewEntry.Item = TSoftObjectPtr<UATGItemData>(FSoftObjectPath(SavedEntry.ItemAssetPath));

		NewEntry.Quantity = SavedEntry.Quantity;
		NewEntry.X = SavedEntry.X;
		NewEntry.Y = SavedEntry.Y;
		NewEntry.bRotated = SavedEntry.bRotated;
		//NewEntry.Id = SavedEntry.Id;

		if (UATGItemData* ItemData = NewEntry.Item.LoadSynchronous())
		{
			NewEntry.Width = ItemData->Width;  
			NewEntry.Height = ItemData->Height;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ConvertJsonToGrid : Load Item Fail !!!"));
			continue;
		}

		int32 Qty = NewEntry.Quantity;
		int32 X = NewEntry.X;
		int32 Y = NewEntry.Y;
		int32 W = NewEntry.Item->Width;
		int32 H = NewEntry.Item->Height;
		bool bRotated = NewEntry.bRotated;
		OutGrid.AddItemAt(NewEntry.Item, Qty, X, Y, W, H, bRotated);

	}
	OutGrid.MarkArrayDirty();

	// 중요: GlobalEntryIdCounter는 AddItemAt에서 생성

	return true;
}
