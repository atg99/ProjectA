import unreal
import csv
import os
import re
import json

def cast_value(target_type, value_str):
    """
    CSV에서 읽어온 문자열을 타겟 언리얼 프로퍼티 타입에 맞게 변환합니다.
    """
    if target_type == bool:
        if isinstance(value_str, str):
            return value_str.lower() in ('true', '1', 'yes')
        return bool(value_str)
    elif target_type == int:
        # Enum value processing: <Enum.Name: 10> -> 10
        if isinstance(value_str, str):
            enum_match = re.search(r':\s*(\d+)>$', value_str)
            if enum_match:
                 return int(enum_match.group(1))
            # Handle standard int/float strings
            if '.' in value_str:
                 return int(float(value_str))
        return int(value_str)
    elif target_type == float:
        return float(value_str)
    elif target_type == unreal.Name:
        return unreal.Name(value_str)
    elif target_type == unreal.Text:
        return unreal.Text(value_str)
    elif target_type == str:
        return value_str
    elif target_type == unreal.GameplayTagContainer:
        # Raw Struct String: <Struct 'GameplayTagContainer' ... {gameplay_tags: ((TagName="A"), (TagName="B"))}>
        # Clean JSON: ["TagName1", "TagName2"] or "TagName1,TagName2"
        container = unreal.GameplayTagContainer()
        
        # 1. Try Simple Comma Split (Legacy/Simple)
        if not value_str or value_str.lower() == "none" or value_str == "":
             pass
        elif value_str.strip().startswith('<Struct'):
             # Extract TagName="..."
             matches = re.findall(r'TagName="([^"]+)"', value_str)
             for t in matches:
                 container.add_tag(unreal.GameplayTag.request_gameplay_tag(unreal.Name(t)))
        elif value_str.strip().startswith('['):
             # JSON List
             try:
                 tags = json.loads(value_str)
                 for t in tags:
                     container.add_tag(unreal.GameplayTag.request_gameplay_tag(unreal.Name(str(t))))
             except:
                 pass
        else:
            # Comma separated
            tags = value_str.split(',')
            for t in tags:
                t = t.strip()
                if t:
                    container.add_tag(unreal.GameplayTag.request_gameplay_tag(unreal.Name(t)))
        return container

    # Enum Handling (Generic)
    try:
        if issubclass(target_type, unreal.EnumBase):
             # <EnumName.Key: Value> => We can cast int to Enum
             # extract int value
             if isinstance(value_str, str):
                 enum_match = re.search(r':\s*(\d+)>$', value_str)
                 if enum_match:
                     val_int = int(enum_match.group(1))
                     return target_type(val_int)
                 # Try to use label if integer parse failed
                 # ex: Enum.ValueName
                 if '.' in value_str:
                      short_name = value_str.split('.')[-1]
                      # Enums usually can be initialized/cast from string/int, but casting from string name isn't always direct in Python API without knowing proper name.
                      # Ideally we rely on the int value which is safer.
    except TypeError:
        # target_type might not be a class
        pass

    # Struct/Array Handling (JSON)
    if isinstance(value_str, str) and (value_str.strip().startswith('{') or value_str.strip().startswith('[')):
        try:
             # Try JSON load
             json_data = json.loads(value_str)
             
             # If target is Array?
             # Unreal Python API: usually we pass the list/dict directly to set_editor_property.
             # We can't easy 'cast' to unreal.Array(Type) explicitly in Python without some gymnastics.
             # Returning the pure Python list/dict often works for set_editor_property!
             return json_data
        except json.JSONDecodeError:
             pass

    return value_str

def export_assets_to_csv(folder_path, csv_file_path, asset_class=None):
    """
    지정된 폴더의 에셋 프로퍼티를 CSV로 내보냅니다.
    """
    asset_lib = unreal.EditorAssetLibrary()
    assets_path = asset_lib.list_assets(folder_path, recursive=True)
    
    rows = []
    headers = set()
    headers.add("AssetPath") 

    for path in assets_path:
        asset_data = asset_lib.load_asset(path)
        
        if asset_class and not isinstance(asset_data, asset_class):
            continue
            
        row_data = {"AssetPath": path}
        
        # [NEW] PrimaryAssetId 추출 (PrimaryAssetId Extraction)
        # 여러 가지 방법으로 PrimaryAssetId 가져오기 시도 (Try multiple ways to get PrimaryAssetId)
        try:
            p_type = None
            p_name = None
            
            # 1. SystemLibrary 사용 (Use SystemLibrary) -> 이미 실패 확인됨 (Already known to fail)
            # if hasattr(unreal.SystemLibrary, 'get_primary_asset_id_for_object'): ...

            # 2. AssetData 구조체 사용 (Use AssetData struct)
            ad_struct = asset_lib.find_asset_data(path)
            if ad_struct:
                # 2-1. Python 속성 직접 접근 (snake_case)
                if hasattr(ad_struct, 'primary_asset_type'):
                    p_type = ad_struct.primary_asset_type
                    p_name = ad_struct.primary_asset_name
                    # unreal.log(f"Method 2-1 (Python/snake_case) succeeded for {path}: {p_type}:{p_name}")
                
                # 2-2. Python 속성 직접 접근 (PascalCase) - 일부 구버전 (Some old versions)
                elif hasattr(ad_struct, 'PrimaryAssetType'):
                    p_type = ad_struct.PrimaryAssetType
                    p_name = ad_struct.PrimaryAssetName
                    # unreal.log(f"Method 2-2 (Python/PascalCase) succeeded for {path}: {p_type}:{p_name}")
                
                # 2-3. Asset Registry Tag 값 확인 (Check Asset Registry Tags)
                else:
                    # 태그 값은 보통 Name이나 String으로 반환됨
                    # Tag values are usually returned as Name or String
                    # 'PrimaryAssetType', 'PrimaryAssetName' 태그가 존재하는지 확인
                    try:
                       p_type = ad_struct.get_tag_value('PrimaryAssetType')
                       p_name = ad_struct.get_tag_value('PrimaryAssetName')
                       # unreal.log(f"Method 2-3 (Asset Registry Tag) succeeded for {path}: {p_type}:{p_name}")
                    except:
                        pass
            
            # 값이 유효한지 체크 (Check if values are valid)
            if p_type and str(p_type) != "None":
                row_data["PrimaryAssetId"] = f"{p_type}:{p_name}"
                headers.add("PrimaryAssetId")
                # 성공 로그 (Success Log)
                unreal.log(f"PrimaryAssetId extracted: {p_type}:{p_name} ({path})")
            else:
                # 디버깅용 로그 (나중에 주석 처리 가능)
                # ue.log_warning(f"No PrimaryAssetId for {path}")
                pass

        except Exception as e:
            unreal.log_warning(f"Error extracting PrimaryAssetId for {path}: {e}")
        
        # 프로퍼티 목록 추출 (Extraction of property list)
        prop_names = []
        try:
            # 1. 표준 방식: get_editor_properties() 사용
            # Standard way: Use get_editor_properties()
            props = asset_data.get_class().get_editor_properties()
            prop_names = [p.get_name() for p in props]
        except AttributeError:
            # 2. 폴백: dir()을 사용하여 속성 검사 (메서드 및 내부 속성 제외)
            # Fallback: Inspect attributes using dir() (exclude methods and internal attributes)
            all_attrs = dir(asset_data)
            reserved_names = [
                'get_class', 'get_name', 'get_full_name', 'get_path_name', 
                'get_outer', 'set_editor_property', 'get_editor_property', 
                'modify', 'rename', 'call_method'
            ]
            
            for attr in all_attrs:
                if attr.startswith('_'):
                    continue
                if attr in reserved_names:
                    continue
                # 함수/메서드는 제외 (Exclude functions/methods)
                if callable(getattr(asset_data, attr)):
                    continue
                prop_names.append(attr)

        for prop_name in prop_names:
            if prop_name in ["None", "OriginalClass", "Class"]: 
                continue
                
            val = asset_data.get_editor_property(prop_name)
            
            # JSON 호환 변환 처리 (JSON compatible conversion)
            val = convert_to_json_compatible(val)
            
            row_data[prop_name] = val
            headers.add(prop_name)
        
        rows.append(row_data)

    # 디렉토리 생성 (Create directory if not exists)
    directory = os.path.dirname(csv_file_path)
    if directory and not os.path.exists(directory):
        try:
            os.makedirs(directory)
        except OSError as e:
            unreal.log_error(f"Failed to create directory {directory}: {e}")
            return

    try:
        with open(csv_file_path, mode='w', newline='', encoding='utf-8') as file:
            # 우선순위 헤더 목록 (Display common/important fields first)
            priority_headers = [
                "AssetPath", 
                "PrimaryAssetId", 
                "item_id", 
                "display_name", 
                "item_type",
                "equipment_type",
                "width",
                "height", 
                "max_stack",
                "icon",
                "mesh",
                "description"
            ]
            
            # headers 세트에 있는 필드 중 우선순위 목록에 있는 것들만 순서대로 추출
            ordered_priority = [h for h in priority_headers if h in headers]
            
            # 나머지 필드는 알파벳순 정렬
            rest_headers = sorted([h for h in headers if h not in ordered_priority])
            
            fieldnames = ordered_priority + rest_headers
            writer = csv.DictWriter(file, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(rows)
        unreal.log(f"Success: Exported {len(rows)} assets to {csv_file_path}")
    except Exception as e:
        unreal.log_error(f"Failed to write CSV: {e}")

def convert_to_json_compatible(val):
    import json
    
    if val is None:
        return ""
        
    if isinstance(val, bool):
        return val # CSV will handle True/False or we can force str if needed
    
    if isinstance(val, (int, float)):
        return val

    if isinstance(val, unreal.Object):
        return val.get_path_name()
    
    if isinstance(val, (unreal.Name, unreal.Text)):
        return str(val)
    
    if isinstance(val, unreal.GameplayTagContainer):
        try:
            # to_list()가 존재하는 경우
            tags = val.to_list()
            return ",".join([str(t) for t in tags])
        except AttributeError:
            return str(val)

    if isinstance(val, unreal.EnumBase): 
        return str(val) # EnumClass.Value

    # Array 처리 (Handling Arrays)
    if isinstance(val, (list, unreal.Array)):
        # 재귀적으로 처리 (Recursive logic)
        return json.dumps([convert_to_json_compatible(item) for item in val])
        
    # Struct 처리 (Handling Structs)
    if isinstance(val, unreal.StructBase):
        # 구조체의 각 프로퍼티를 딕셔너리로 변환
        struct_dict = {}
        # StructBase는 dir()로 프로퍼티 확인 가능
        # Note: 일부 버전에서는 속성 접근이 다를 수 있음
        try:
             # dir()을 이용하여 속성 순회 (Inspect properties via dir())
             for attr in dir(val):
                 if attr.startswith('_') or attr in ['make_struct', 'to_tuple', 'cast', 'static_struct']:
                     continue
                 if callable(getattr(val, attr)):
                     continue
                 
                 child_val = getattr(val, attr)
                 struct_dict[attr] = convert_to_json_compatible(child_val)
             return json.dumps(struct_dict)
        except:
             return str(val)
             
    # Map 처리 (Handling Maps)
    if isinstance(val, (dict, unreal.Map)):
         new_dict = {}
         for k, v in val.items():
             new_k = str(k) # 키는 문자열로 가정
             new_dict[new_k] = convert_to_json_compatible(v)
         return json.dumps(new_dict)

    return str(val)

def import_csv_to_assets(csv_file_path):
    """
    CSV 파일을 읽어 에셋 값을 업데이트합니다.
    """
    if not os.path.exists(csv_file_path):
        unreal.log_error("CSV file not found.")
        return

    asset_lib = unreal.EditorAssetLibrary()
    updated_count = 0

    try:
        with open(csv_file_path, mode='r', encoding='utf-8') as file:
            reader = csv.DictReader(file)
            
            with unreal.ScopedSlowTask(100, "Importing CSV Data...") as slow_task:
                slow_task.make_dialog()
                
                for row in reader:
                    asset_path = row.get("AssetPath")
                    if not asset_path or not asset_lib.does_asset_exist(asset_path):
                        unreal.log_warning(f"Asset not found: {asset_path}")
                        continue
                    
                    asset_obj = asset_lib.load_asset(asset_path)
                    is_dirty = False
                    
                    for prop_name, value_str in row.items():
                        if prop_name == "AssetPath":
                            continue
                            
                        try:
                            # 현재 값으로 타입 추론
                            current_value = asset_obj.get_editor_property(prop_name)
                            target_type = type(current_value)
                            
                            new_value = cast_value(target_type, value_str)
                            
                            # 값 비교 및 업데이트
                            # 문자열 변환 비교가 가장 안전함 (특히 객체나 구조체)
                            if str(current_value) != str(new_value):
                                # GameplayTagContainer 등은 str() 비교가 정확하지 않을 수 있으나 여기선 단순화
                                asset_obj.set_editor_property(prop_name, new_value)
                                is_dirty = True
                        except Exception as e:
                            pass
                    
                    if is_dirty:
                        asset_lib.save_loaded_asset(asset_obj)
                        updated_count += 1
                        
        unreal.log(f"Success: Updated {updated_count} assets from CSV.")
        
    except Exception as e:
        unreal.log_error(f"Failed to read CSV: {e}")

# --- 실행 예시 ---
# ATGItemData 추출 예시
# export_assets_to_csv("/Game/Data/Items", "C:/Temp/ItemData.csv", unreal.ATGItemData)
