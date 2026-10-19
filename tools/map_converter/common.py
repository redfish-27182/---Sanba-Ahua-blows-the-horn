"""地圖工具與瓦片工具共用的格式及檔案驗證。"""

import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parent
INPUT_DIR = ROOT / "input"
OUTPUT_DIR = ROOT / "output"
TILE_SIZE = 16
MAX_TILE_CODE = 8191  # 13 bits；0 保留為空格。
FORMAT_VERSION = 2
LIBRARY_HEADER = struct.Struct("<4sHHI16s")
MAP_HEADER = struct.Struct("<4sHHHH16s")


class ConversionError(Exception):
    """可以直接顯示給使用者的轉檔錯誤。"""


def read_json(path):
    try:
        with path.open("r", encoding="utf-8") as file:
            return json.load(file)
    except (OSError, ValueError) as exc:
        raise ConversionError(f"無法讀取 {path.name}，請先完成前一步驟。") from exc


def write_json(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def read_library(output_root):
    """讀取並驗證已存在的素材庫及電腦端對照表。"""
    index = read_json(output_root / "tileset.index.json")
    try:
        data = (output_root / "tileset.bin").read_bytes()
        magic, version, tile_size, count, identifier = LIBRARY_HEADER.unpack_from(data)
        id_bytes = data[LIBRARY_HEADER.size:LIBRARY_HEADER.size + count * 2]
        pixels = data[LIBRARY_HEADER.size + count * 2:]
        if len(id_bytes) != count * 2:
            raise ValueError("incomplete id table")
        ids = list(struct.unpack(f"<{count}H", id_bytes))
        expected_length = count * TILE_SIZE * TILE_SIZE * 2
        if (magic != b"AHTS" or version != FORMAT_VERSION or tile_size != TILE_SIZE
                or count > MAX_TILE_CODE or len(pixels) != expected_length
                or ids != sorted(set(ids)) or any(item >= MAX_TILE_CODE for item in ids)
                or hashlib.sha256(id_bytes + pixels).digest()[:16] != identifier
                or index.get("version") != FORMAT_VERSION
                or index.get("library_id") != identifier.hex()
                or index.get("tile_ids") != ids):
            raise ValueError("inconsistent library")
        columns = index["columns"]
        rows = index["rows"]
        if not isinstance(columns, int) or not isinstance(rows, int) or columns < 1 or rows < 1:
            raise ValueError("invalid image dimensions")
        if columns * rows > MAX_TILE_CODE or any(item >= columns * rows for item in ids):
            raise ValueError("invalid tile index")
    except (OSError, ValueError, TypeError, KeyError, struct.error, AttributeError) as exc:
        raise ConversionError("瓦片庫或對照表不存在、不一致或損壞，請重新執行瓦片工具。") from exc
    return {"identifier": identifier, "ids": ids, "pixels": pixels, "index": index}
