"""將 UTF-8 字元清單轉成 ESP32 可讀取的單色 16×16 點陣字庫。"""

import argparse
import os
import struct
import tempfile
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTFont, TTLibError

ROOT = Path(__file__).resolve().parent
HEADER = struct.Struct("<4sHHHHI")
RECORD = struct.Struct("<I32s")
SIZE = 16
FALLBACK = "□"


def read_characters(path):
    """讀取 UTF-8 清單、去除控制字元及重複字，補上缺字標記。"""
    text = path.read_text(encoding="utf-8-sig")
    chars = {ch for ch in text if ch == " " or not ch.isspace()}
    chars.update(chr(code) for code in range(32, 127))
    chars.add(FALLBACK)
    return sorted(chars, key=ord)


def supported_characters(font_path):
    """讀取字型 Unicode 對照表，避免把缺字圖案誤當成真正字形。"""
    with TTFont(font_path, lazy=True, fontNumber=0) as font:
        return set(font.getBestCmap() or {})


def render_glyph(font, char, threshold):
    """把單一字形置中於 16×16 畫布，轉為逐行、高位在前的 1-bit 資料。"""
    image = Image.new("L", (SIZE, SIZE), 0)
    draw = ImageDraw.Draw(image)
    draw.text((SIZE // 2, SIZE // 2), char, font=font, fill=255, anchor="mm")
    pixels = image.load()
    result = bytearray()
    for y in range(SIZE):
        for group in range(2):
            value = 0
            for bit in range(8):
                value = (value << 1) | (pixels[group * 8 + bit, y] >= threshold)
            result.append(value)
    return bytes(result)


def export_font(font_path, chars_path, output_path, threshold=128):
    """建立排序後的 Unicode 與點陣紀錄，成功後才取代輸出檔。"""
    if not 1 <= threshold <= 255:
        raise ValueError("點陣亮度門檻必須在 1 到 255 之間。")
    chars = read_characters(chars_path)
    cmap = supported_characters(font_path)
    missing = [char for char in chars if ord(char) not in cmap]
    if missing:
        raise ValueError(f"字型缺少 {len(missing)} 個字元：{''.join(missing[:30])}")
    font = ImageFont.truetype(str(font_path), SIZE)
    data = bytearray(HEADER.pack(b"AHFT", 1, SIZE, SIZE, 0, len(chars)))
    for char in chars:
        data.extend(RECORD.pack(ord(char), render_glyph(font, char, threshold)))
    output_path.parent.mkdir(parents=True, exist_ok=True)
    # 暫存檔與輸出放在同個資料夾，避免轉檔失敗時留下不完整字庫。
    temp_path = None
    try:
        with tempfile.NamedTemporaryFile(dir=output_path.parent, delete=False) as temp:
            temp_path = Path(temp.name)
            temp.write(data)
        os.replace(temp_path, output_path)
    finally:
        if temp_path and temp_path.exists():
            temp_path.unlink()
    print(f"已輸出 {output_path.name}：{len(chars)} 字，{len(data)} bytes")


def main():
    """讀取命令列路徑並轉換 system 或 common 字庫。"""
    parser = argparse.ArgumentParser(description="將中文字型轉為 16×16 單色字庫")
    parser.add_argument("kind", choices=("system", "common"), help="字庫種類")
    parser.add_argument("--font", default=str(ROOT / "input" / "NotoSansCJKtc-Regular.otf"), help="OTF 或 TTF 字型檔")
    parser.add_argument("--chars", help="自訂 UTF-8 字元清單")
    parser.add_argument("--threshold", type=int, default=128, help="點陣門檻，預設 128")
    args = parser.parse_args()
    try:
        export_font(Path(args.font), Path(args.chars) if args.chars else ROOT / "characters" / f"{args.kind}.txt", ROOT / "output" / f"{args.kind}.bin", args.threshold)
    except (OSError, ValueError, KeyError, TTLibError) as exc:
        parser.exit(1, f"字庫處理失敗：{exc}\n")


if __name__ == "__main__":
    main()
