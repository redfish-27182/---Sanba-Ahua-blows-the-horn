#!/usr/bin/env python3
"""只讀取 input 中的 .tmx，掃描需求及輸出地圖二進位檔。"""

import argparse
import hashlib
import re
import struct
import sys
import xml.etree.ElementTree as ET

from common import (ConversionError, FORMAT_VERSION, INPUT_DIR, MAP_HEADER,
                    MAX_TILE_CODE, OUTPUT_DIR, TILE_SIZE, read_json, read_library,
                    write_json)

TILED_H = 0x80000000
TILED_V = 0x40000000
TILED_D = 0x20000000
TILED_GID_MASK = 0x0FFFFFFF  # 一併清除 Tiled 的四個高位旗標。
MAP_H = 1 << 13
MAP_V = 1 << 14
MAP_D = 1 << 15


def check_layer(element, map_name):
    name = element.get("name", element.tag)
    if element.get("visible", "1") != "1" or float(element.get("opacity", "1")) != 1:
        raise ConversionError(f"{map_name} 的圖層「{name}」隱藏或不透明度不是 100%。")
    if (element.get("tintcolor") or float(element.get("parallaxx", "1")) != 1
            or float(element.get("parallaxy", "1")) != 1):
        raise ConversionError(f"{map_name} 的圖層「{name}」有色調或視差，目前不支援。")
    if float(element.get("offsetx", "0")) or float(element.get("offsety", "0")):
        # 按目前的地圖規則，輸出時固定為 0，不修改 Tiled 檔案。
        print(f"提示：{map_name} 的「{name}」偏移在輸出時設為 0, 0。")
    if int(element.get("x", "0")) or int(element.get("y", "0")):
        raise ConversionError(f"{map_name} 的圖層「{name}」有格子座標偏移，目前不支援。")


def read_layers(parent, width, height, map_name):
    layers = []
    for element in parent:
        if element.tag == "group":
            check_layer(element, map_name)
            layers.extend(read_layers(element, width, height, map_name))
        elif element.tag == "layer":
            check_layer(element, map_name)
            if (int(element.get("width", "0")), int(element.get("height", "0"))) != (width, height):
                raise ConversionError(f"{map_name} 的圖層尺寸與地圖不一致。")
            data = element.find("data")
            if data is None or data.get("encoding") != "csv" or data.get("compression"):
                raise ConversionError(f"{map_name} 只支援未壓縮的 CSV 瓦片圖層。")
            values = [int(value) for value in re.split(r"[,\s]+", data.text or "") if value]
            if len(values) != width * height or any(value < 0 or value > 0xFFFFFFFF for value in values):
                raise ConversionError(f"{map_name} 的圖層格數或瓦片 ID 不正確。")
            layers.append(values)
        elif element.tag in ("objectgroup", "imagelayer"):
            raise ConversionError(f"{map_name} 含 {element.tag}，第一版只支援瓦片圖層。")
    return layers


def cell_from_gid(gid, first_gid):
    """轉換 Tiled GID；低 13 bits 是原始瓦片編號加一，高 3 bits 是翻轉。"""
    if gid == 0:
        return 0
    raw_id = gid & TILED_GID_MASK
    local_id = raw_id - first_gid
    if raw_id == 0 or not 0 <= local_id < MAX_TILE_CODE:
        raise ConversionError(f"Tiled GID {gid} 超出可表示的瓦片範圍。")
    return ((local_id + 1) | (MAP_H if gid & TILED_H else 0)
            | (MAP_V if gid & TILED_V else 0) | (MAP_D if gid & TILED_D else 0))


def load_maps(input_root):
    """不開啟 .tsx 或 .png；只用 .tmx 的 firstgid 與圖層資料。"""
    root_dir = input_root.resolve()
    paths = sorted(path for path in root_dir.rglob("*.tmx")
                   if path.is_file() and path.resolve().is_relative_to(root_dir))
    if not paths:
        raise ConversionError("input 裡找不到 .tmx 地圖。")
    if len({path.stem.casefold() for path in paths}) != len(paths):
        raise ConversionError("input 裡有同名地圖，請先重新命名。")
    if any(path.stem.casefold() == "tileset" for path in paths):
        raise ConversionError("地圖不能取名 tileset.tmx，會與瓦片庫輸出名稱衝突。")
    maps, sheet_names, used_ids = [], set(), set()
    for path in paths:
        raw = path.read_bytes()
        try:
            root = ET.fromstring(raw)
        except ET.ParseError as exc:
            raise ConversionError(f"{path.name} 不是有效的 TMX：{exc}") from exc
        if root.tag != "map" or root.get("orientation") != "orthogonal" or root.get("infinite", "0") != "0":
            raise ConversionError(f"{path.name} 必須是有限大小的正交地圖。")
        if (int(root.get("tilewidth", "0")), int(root.get("tileheight", "0"))) != (TILE_SIZE, TILE_SIZE):
            raise ConversionError(f"{path.name} 必須使用 16 × 16 瓦片。")
        width, height = int(root.get("width", "0")), int(root.get("height", "0"))
        if not (0 < width <= 65535 and 0 < height <= 65535):
            raise ConversionError(f"{path.name} 的尺寸無效。")
        references = root.findall("tileset")
        if len(references) != 1:
            raise ConversionError(f"{path.name} 第一版只支援一個瓦片集。")
        first_gid = int(references[0].get("firstgid", "0"))
        if first_gid < 1:
            raise ConversionError(f"{path.name} 的 firstgid 無效。")
        # 此名稱只用於檢查多張地圖是否指向同一個瓦片集，不開啟其檔案。
        source = references[0].get("source") or references[0].get("name")
        if not source:
            raise ConversionError(f"{path.name} 無法確認瓦片集名稱。")
        sheet_names.add(source.replace("\\", "/").rsplit("/", 1)[-1].casefold())
        layers = read_layers(root, width, height, path.name)
        if not layers or len(layers) > 65535:
            raise ConversionError(f"{path.name} 沒有可用的瓦片圖層。")
        cells = [[cell_from_gid(gid, first_gid) for gid in layer] for layer in layers]
        used_ids.update((cell & MAX_TILE_CODE) - 1 for layer in cells for cell in layer if cell)
        maps.append({"path": path, "relative_path": path.relative_to(root_dir).as_posix(),
                     "sha256": hashlib.sha256(raw).hexdigest(), "width": width,
                     "height": height, "layers": cells})
    if len(sheet_names) != 1:
        raise ConversionError("多張地圖須使用同一個瓦片集；目前不支援多張 PNG。")
    return maps, sorted(used_ids)


def requirements_for(maps, ids):
    return {"version": FORMAT_VERSION, "tile_size": TILE_SIZE, "tile_ids": ids,
            "maps": [{"path": item["relative_path"], "sha256": item["sha256"]} for item in maps]}


def scan(input_root, output_root):
    maps, ids = load_maps(input_root)
    output_root.mkdir(parents=True, exist_ok=True)
    write_json(output_root / "tile_requirements.json", requirements_for(maps, ids))
    print(f"已掃描 {len(maps)} 張地圖，使用 {len(ids)} 種原始瓦片；接著請執行瓦片工具。")


def export_maps(input_root, output_root):
    maps, ids = load_maps(input_root)
    if read_json(output_root / "tile_requirements.json") != requirements_for(maps, ids):
        raise ConversionError("掃描後地圖或內容已變更；請重新執行 map_tool.py scan。")
    library = read_library(output_root)
    missing = sorted(set(ids) - set(library["ids"]))
    if missing:
        raise ConversionError(f"既有瓦片庫缺少 {len(missing)} 種原始瓦片；請重新執行瓦片工具。")
    outputs = {}
    for item in maps:
        header = MAP_HEADER.pack(b"AHMP", FORMAT_VERSION, item["width"], item["height"],
                                 len(item["layers"]), library["identifier"])
        cells = b"".join(struct.pack("<H", cell) for layer in item["layers"] for cell in layer)
        outputs[item["path"].stem + ".bin"] = header + cells
    for name, data in outputs.items():
        (output_root / name).write_bytes(data)
        print(f"已輸出 {name}：{len(data)} bytes")
    print(f"瓦片庫識別碼：{library['identifier'].hex()}")


def main():
    parser = argparse.ArgumentParser(description="掃描 TMX 地圖，或依現有瓦片庫產生地圖檔。")
    parser.add_argument("action", choices=("scan", "export"), help="scan：掃描需求；export：輸出地圖")
    args = parser.parse_args()
    try:
        if args.action == "scan":
            scan(INPUT_DIR, OUTPUT_DIR)
        else:
            export_maps(INPUT_DIR, OUTPUT_DIR)
    except (ConversionError, OSError, ValueError) as exc:
        print(f"地圖處理失敗：{exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
