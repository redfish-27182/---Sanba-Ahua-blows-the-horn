"""使用測試資料檢查字庫格式，不讀取使用者地圖或輸入資料夾。"""

import os
import struct
import tempfile
import unittest
from pathlib import Path

from PIL import ImageFont
from font_tool import HEADER, RECORD, export_font, read_characters, render_glyph


class FontToolTests(unittest.TestCase):
    def test_characters_are_sorted_unique_and_include_fallback(self):
        with tempfile.TemporaryDirectory() as root:
            path = Path(root) / "chars.txt"
            path.write_text("你好你好\n", encoding="utf-8")
            chars = read_characters(path)
            self.assertEqual(chars, sorted(set(chars), key=ord))
            self.assertIn("□", chars)
            self.assertIn("A", chars)
            self.assertNotIn("\n", chars)

    def test_glyph_is_32_bytes_and_space_is_empty(self):
        font = ImageFont.load_default(size=16)
        self.assertEqual(len(render_glyph(font, "A", 128)), 32)
        self.assertEqual(render_glyph(font, " ", 128), bytes(32))

    @unittest.skipUnless(os.name == "nt" and (Path(os.environ.get("WINDIR", "")) / "Fonts" / "arial.ttf").exists(), "需要 Windows Arial 測試字型")
    def test_export_format_and_missing_character(self):
        font = Path(os.environ["WINDIR"]) / "Fonts" / "arial.ttf"
        with tempfile.TemporaryDirectory() as root:
            chars = Path(root) / "chars.txt"
            output = Path(root) / "font.bin"
            chars.write_text("AB", encoding="utf-8")
            export_font(font, chars, output)
            data = output.read_bytes()
            magic, version, width, height, reserved, count = HEADER.unpack_from(data)
            self.assertEqual((magic, version, width, height, reserved), (b"AHFT", 1, 16, 16, 0))
            self.assertEqual(len(data), HEADER.size + count * RECORD.size)
            ids = [struct.unpack_from("<I", data, HEADER.size + i * RECORD.size)[0] for i in range(count)]
            self.assertEqual(ids, sorted(set(ids)))
            self.assertIn(ord("□"), ids)
            original = data
            chars.write_text(chr(0x10FFFF), encoding="utf-8")
            with self.assertRaises(ValueError):
                export_font(font, chars, output)
            self.assertEqual(output.read_bytes(), original)


if __name__ == "__main__":
    unittest.main()
