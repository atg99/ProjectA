import unreal
import csv
import os

def cast_value(target_type, value_str):
    """
    CSV에서 읽어온 문자열을 타겟 언리얼 프로퍼티 타입에 맞게 변환합니다.
    Converts the string read from CSV to match the target Unreal property type.
    """
    if target_type == bool:
        return value_str.lower() in ('true', '1', 'yes')
    elif target_type == int:
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
        # "Tag.A, Tag.B" 형식의 문자열을 컨테이너로 변환
        # Convert "Tag.A, Tag.B" format string to container
        container = unreal.GameplayTagContainer()
        if value_str:
            tags = value_str.split(',')
            for t in tags:
                t = t.strip()
                if t:
                    # 태그가 존재하지 않으면 경고가 뜰 수 있음
                    # Warning may appear if tag does not exist
                    tag_struct = unreal.GameplayTag.request_gameplay_tag(unreal.Name(t))
                    container.add_tag(tag_struct)
        return container
        
    return value_str

def import_csv_to_assets(csv_file_path):
    """
    CSV 파일을 읽어 에셋 값을 업데이트합니다.
    Reads CSV file and updates asset values.
    """
    if not os.path.exists(csv_file_path):
        unreal.log_error(f"CSV file not found: {csv_file_path}")
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
                        if prop_name == "AssetPath" or prop_name == "PrimaryAssetId":
                            # AssetPath와 PrimaryAssetId는 수정 대상이 아님
                            # AssetPath and PrimaryAssetId are not targets for modification
                            continue
                            
                        try:
                            # 현재 값으로 타입 추론
                            # Infer type from current value
                            current_value = asset_obj.get_editor_property(prop_name)
                            target_type = type(current_value)
                            
                            new_value = cast_value(target_type, value_str)
                            
                            # 값 비교 및 업데이트
                            # Compare and update values
                            # 문자열 변환 비교가 가장 안전함 (특히 객체나 구조체)
                            # String conversion comparison is safest (especially for objects or structs)
                            if str(current_value) != str(new_value):
                                # GameplayTagContainer 등은 str() 비교가 정확하지 않을 수 있으나 여기선 단순화
                                # GameplayTagContainer etc. might not be accurate with str() comparison but simplified here
                                asset_obj.set_editor_property(prop_name, new_value)
                                is_dirty = True
                        except Exception as e:
                            # 프로퍼티가 없거나 타입 변환 실패시 무시 (로그 필요시 추가)
                            # Ignore if property missing or type conversion fails (add log if needed)
                            pass
                    
                    if is_dirty:
                        asset_lib.save_loaded_asset(asset_obj)
                        updated_count += 1
                        
        unreal.log(f"Success: Updated {updated_count} assets from CSV.")
        
    except Exception as e:
        unreal.log_error(f"Failed to read CSV: {e}")

if __name__ == "__main__":
    import argparse
    import sys

    # Argument Parser Setup
    parser = argparse.ArgumentParser(description="Import CSV Data to Unreal Assets")
    parser.add_argument("csv_path", type=str, help="Input CSV File Path (e.g., C:/Temp/ItemData.csv)")

    if len(sys.argv) > 1:
        args, unknown = parser.parse_known_args()
        CSV_PATH = args.csv_path
        import_csv_to_assets(CSV_PATH)
    else:
        # Default behavior
        unreal.log_warning("No arguments provided. Using default hardcoded path.")
        DEFAULT_CSV_PATH = "C:/Temp/ItemData.csv"
        import_csv_to_assets(DEFAULT_CSV_PATH)
