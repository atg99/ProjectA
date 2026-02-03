import unreal
import csv
import os

def export_assets_to_csv(folder_path, csv_file_path, asset_class=None):
    """
    지정된 폴더의 에셋 프로퍼티를 CSV로 내보냅니다.
    Exports asset properties of a specified folder to CSV.
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
        
        # [NEW] Check PrimaryAssetId
        primary_id = unreal.SystemLibrary.get_primary_asset_id_for_object(asset_data)
        if primary_id.is_valid():
            # Usually saved as "Type:Name" string in DB
            row_data["PrimaryAssetId"] = f"{primary_id.primary_asset_type}:{primary_id.primary_asset_name}"
            headers.add("PrimaryAssetId")
        
        for prop in asset_data.get_class().get_editor_properties():
            prop_name = prop.get_name()
            if prop_name in ["None", "OriginalClass", "Class"]: 
                continue
                
            val = asset_data.get_editor_property(prop_name)
            
            # Type conversion processing
            if isinstance(val, unreal.Object):
                val = val.get_path_name()
            elif isinstance(val, unreal.Text):
                val = val.to_string()
            elif isinstance(val, unreal.GameplayTagContainer):
                # Convert to Tag1, Tag2 format
                tags = val.to_list()
                val = ",".join([str(t) for t in tags])
            elif isinstance(val, unreal.EnumBase): # Enum processing
                val = str(val) # Usually comes out as EnumClass.Value
            
            row_data[prop_name] = str(val)
            headers.add(prop_name)
            
        rows.append(row_data)

    try:
        # Check if directory exists, if not create it
        directory = os.path.dirname(csv_file_path)
        if directory and not os.path.exists(directory):
            os.makedirs(directory)

        with open(csv_file_path, mode='w', newline='', encoding='utf-8') as file:
            # Place AssetPath at the front
            fieldnames = ["AssetPath"] + sorted([h for h in headers if h != "AssetPath"])
            writer = csv.DictWriter(file, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(rows)
        unreal.log(f"Success: Exported {len(rows)} assets to {csv_file_path}")
    except Exception as e:
        unreal.log_error(f"Failed to write CSV: {e}")

if __name__ == "__main__":
    import argparse
    import sys

    # Argument Parser Setup
    parser = argparse.ArgumentParser(description="Export Unreal Assets to CSV")
    parser.add_argument("folder_path", type=str, help="Unreal Content Folder Path (e.g., /Game/Data/Items)")
    parser.add_argument("csv_path", type=str, help="Output CSV File Path (e.g., C:/Temp/ItemData.csv)")
    parser.add_argument("--class_name", type=str, default=None, help="Optional: Specific Unreal Class Name to filter (e.g., ATGItemData)")

    # Check if arguments are passed (sys.argv will have more than 1 element if arguments are passed)
    # If no arguments are passed, fall back to default behavior or raise error
    if len(sys.argv) > 1:
        # Unreal Python environment might pass extra args, we usually parse known args
        # parse_known_args is safer in some embedded environments
        args, unknown = parser.parse_known_args()
        
        folder_path = args.folder_path
        csv_path = args.csv_path
        target_class = None

        if args.class_name:
            try:
                # Try to get the class from 'unreal' module
                target_class = getattr(unreal, args.class_name)
            except AttributeError:
                unreal.log_warning(f"Class 'unreal.{args.class_name}' not found. Exporting all assets.")
                target_class = None
        
        export_assets_to_csv(folder_path, csv_path, target_class)
    else:
        # Default behavior if no arguments provided (Previous logic)
        unreal.log_warning("No arguments provided. Using default hardcoded paths.")
        
        # 기본값 설정 (Defaults)
        DEFAULT_FOLDER_PATH = "/Game/Data/Items"
        DEFAULT_CSV_PATH = "C:/Temp/ItemData.csv"
        
        # Setup Default Class
        try:
            DEFAULT_TARGET_CLASS = unreal.ATGItemData
        except AttributeError:
            DEFAULT_TARGET_CLASS = None
            
        export_assets_to_csv(DEFAULT_FOLDER_PATH, DEFAULT_CSV_PATH, DEFAULT_TARGET_CLASS)
