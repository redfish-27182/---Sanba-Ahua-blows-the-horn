#!/usr/bin/env python3
"""讀取 PNG 與地圖需求清單，輸出沒有翻轉副本的精簡瓦片庫。"""

import argparse
import hashlib
import struct
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("缺少 Pillow，請依 README.md 安裝 requirements.txt。", file=sys.stderr)
    sys.exit(1)

from common import (ConversionError, FORMAT_VERSION, INPUT_DIR, LIBRARY_HEADER,
                    MAX_TILE_CODE, OUTPUT_DIR, PALETTE_COLOR_COUNT, TILE_SIZE,
                    read_json, write_json)

TRANSPARENT_KEY = 0xF81F
ALPHA_THRESHOLD = 128


def choose_png(input_root, filename):
    root = input_root.resolve()
    if filename:
        path = (root / filename).resolve()
        if not path.is_relative_to(root) or not path.is_file() or path.suffix.lower() != ".png":
            raise ConversionError("指定的 PNG 必須是 input 裡存在的檔案。")
        return path
    paths = sorted(path.resolve() for path in root.rglob("*.png")
                   if path.is_file() and path.resolve().is_relative_to(root))
    if len(paths) != 1:
        raise ConversionError("input 必須有一張 PNG；若有多張，請用 --png 指定相對於 input 的路徑。")
    return paths[0]


def encode_tile(image, columns, local_id):
    """只儲存原始方向；半透明像素依門檻轉成透明或不透明。"""
    left = (local_id % columns) * TILE_SIZE
    top = (local_id // columns) * TILE_SIZE
    tile = image.crop((left, top, left + TILE_SIZE, top + TILE_SIZE))
    colors = []
    partial_alpha_count = 0
    for red, green, blue, alpha in tile.getdata():
        if 0 < alpha < 255:
            partial_alpha_count += 1
        color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3)
        if alpha < ALPHA_THRESHOLD:
            color = TRANSPARENT_KEY
        elif color == TRANSPARENT_KEY:
            raise ConversionError(f"瓦片 {local_id} 的不透明顏色與 RGB565 桃紅透明鍵衝突。")
        # 高位元組在前；ESP32 端需配合顯示器的傳送方式驗證。
        colors.append(color)
    return colors, partial_alpha_count


def build_tileset(input_root, output_root, filename=None):
    requirements = read_json(output_root / "tile_requirements.json")
    try:
        ids = requirements["tile_ids"]
        if (requirements["version"] != FORMAT_VERSION or requirements["tile_size"] != TILE_SIZE
                or not isinstance(ids, list) or any(type(item) is not int for item in ids)
                or ids != sorted(set(ids)) or any(item < 0 or item >= MAX_TILE_CODE for item in ids)):
            raise ValueError("invalid requirements")
    except (ValueError, TypeError, KeyError, AttributeError) as exc:
        raise ConversionError("瓦片需求清單無效，請先重新執行地圖掃描。") from exc
    png_path = choose_png(input_root, filename)
    try:
        with Image.open(png_path) as source:
            if source.format != "PNG":
                raise ConversionError("指定的素材必須是 PNG 圖片。")
            image = source.convert("RGBA")
    except (OSError, ValueError) as exc:
        raise ConversionError(f"無法讀取 PNG：{exc}") from exc
    if image.width % TILE_SIZE or image.height % TILE_SIZE:
        raise ConversionError("PNG 寬、高必須是 16 的倍數，且瓦片間不得有間隔或留白。")
    columns, rows = image.width // TILE_SIZE, image.height // TILE_SIZE
    if not columns or not rows or columns * rows > MAX_TILE_CODE:
        raise ConversionError("PNG 的瓦片總數必須介於 1 到 8191 之間。")
    if ids and ids[-1] >= columns * rows:
        raise ConversionError(f"地圖需要原始瓦片 {ids[-1]}，但 PNG 只有 {columns * rows} 片。")

    # 原始瓦片 ID 排序後，與像素順序一一對應，可於 ESP32 端二分搜尋。
    id_bytes = b"".join(struct.pack("<H", tile_id) for tile_id in ids)
    encoded_tiles = [encode_tile(image, columns, tile_id) for tile_id in ids]
    unique_colors = {color for tile_colors, _ in encoded_tiles for color in tile_colors}
    unique_colors.discard(TRANSPARENT_KEY)
    if len(unique_colors) + 1 > PALETTE_COLOR_COUNT:
        raise ConversionError(
            f"素材共有 {len(unique_colors) + 1} 種 RGB565 顏色，超過無損色盤上限 {PALETTE_COLOR_COUNT}。"
        )
    palette = [TRANSPARENT_KEY, *sorted(unique_colors)]
    palette_lookup = {color: index for index, color in enumerate(palette)}
    palette_bytes = b"".join(struct.pack(">H", color) for color in palette)
    palette_bytes += b"\x00" * ((PALETTE_COLOR_COUNT - len(palette)) * 2)
    pixels = b"".join(
        bytes(palette_lookup[color] for color in tile_colors)
        for tile_colors, _ in encoded_tiles
    )
    partial_alpha_count = sum(count for _, count in encoded_tiles)
    identifier = hashlib.sha256(id_bytes + palette_bytes + pixels).digest()[:16]
    header = LIBRARY_HEADER.pack(b"AHTS", FORMAT_VERSION, TILE_SIZE, len(ids), identifier)
    index = {"version": FORMAT_VERSION, "library_id": identifier.hex(),
             "columns": columns, "rows": rows, "tile_ids": ids,
             "palette_color_count": len(palette),
             "png_sha256": hashlib.sha256(png_path.read_bytes()).hexdigest()}

    output_root.mkdir(parents=True, exist_ok=True)
    (output_root / "tileset.bin").write_bytes(header + id_bytes + palette_bytes + pixels)
    write_json(output_root / "tileset.index.json", index)
    print(f"已輸出 tileset.bin：{len(header) + len(id_bytes) + len(palette_bytes) + len(pixels)} bytes")
    print(f"原始瓦片：{len(ids)}；色盤：{len(palette)} 色；精簡對照表：{len(id_bytes)} bytes；瓦片庫識別碼：{identifier.hex()}")
    print(f"已按透明度門檻 {ALPHA_THRESHOLD} 處理 {partial_alpha_count} 個半透明像素（只計算本次使用的原始瓦片）。")
    print("瓦片庫已重建，請將所有使用它的地圖一起重新輸出。")


def main():
    parser = argparse.ArgumentParser(description="以 PNG 及地圖需求清單建立精簡瓦片庫。")
    parser.add_argument("--png", help="PNG 在 input 中的相對路徑；只有一張時可省略")
    args = parser.parse_args()
    try:
        build_tileset(INPUT_DIR, OUTPUT_DIR, args.png)
    except (ConversionError, OSError, ValueError) as exc:
        print(f"瓦片處理失敗：{exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
