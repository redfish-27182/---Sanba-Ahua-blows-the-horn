"""使用臨時合成素材驗證轉檔，不讀取使用者的地圖或圖片。"""

import contextlib
import io
import struct
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image
import common
import map_tool
import tileset_tool


class ToolTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.input = self.root / "input"
        self.output = self.root / "output"
        self.input.mkdir()
        self.image = Image.new("RGBA", (32, 16), (0, 0, 0, 255))
        self.image.putpixel((0, 0), (0, 0, 0, 0))
        self.image.putpixel((17, 2), (255, 0, 0, 255))
        self.save_image()

    def save_image(self):
        self.image.save(self.input / "tiles.png")

    def write_map(self, name="map.tmx", first_gid=1, values=None, offset="0"):
        if values is None:
            values = [first_gid, first_gid + 1, map_tool.TILED_H | (first_gid + 1), 0]
        xml = ('<map orientation="orthogonal" width="2" height="2" tilewidth="16" tileheight="16">'
               f'<tileset firstgid="{first_gid}" source="not-needed.tsx"/>'
               f'<group name="group" offsetx="{offset}">'
               '<layer name="ground" width="2" height="2"><data encoding="csv">'
               + ",".join(str(item) for item in values)
               + '</data></layer></group></map>')
        (self.input / name).write_text(xml, encoding="utf-8")

    def build(self):
        map_tool.scan(self.input, self.output)
        tileset_tool.build_tileset(self.input, self.output)
        map_tool.export_maps(self.input, self.output)

    def test_whole_workflow_and_binary_layout(self):
        self.write_map(offset="57.9")
        self.build()
        library = (self.output / "tileset.bin").read_bytes()
        map_data = (self.output / "map.bin").read_bytes()
        magic, version, size, count, library_id = common.LIBRARY_HEADER.unpack_from(library)
        self.assertEqual((magic, version, size, count), (b"AHTS", common.FORMAT_VERSION, 16, 2))
        self.assertEqual(struct.unpack_from("<2H", library, common.LIBRARY_HEADER.size), (0, 1))
        self.assertEqual(len(library), common.LIBRARY_HEADER.size + 4 + 256 * 2 + 2 * 256)
        map_header = common.MAP_HEADER.unpack_from(map_data)
        self.assertEqual(map_header, (b"AHMP", common.FORMAT_VERSION, 2, 2, 1, library_id))
        self.assertEqual(struct.unpack_from("<4H", map_data, common.MAP_HEADER.size), (1, 2, 2 | map_tool.MAP_H, 0))
        self.assertEqual(library[common.LIBRARY_HEADER.size + 4:common.LIBRARY_HEADER.size + 6], b"\xf8\x1f")
        # 這裡沒有建立 .tsx，證明整個流程不讀取 .tsx。
        self.assertFalse((self.input / "not-needed.tsx").exists())
        self.assertIn('offsetx="57.9"', (self.input / "map.tmx").read_text(encoding="utf-8"))

    def test_all_flip_flag_combinations(self):
        for bits in range(8):
            gid = 7 | (map_tool.TILED_H if bits & 1 else 0) | (map_tool.TILED_V if bits & 2 else 0) | (map_tool.TILED_D if bits & 4 else 0)
            cell = map_tool.cell_from_gid(gid, first_gid=7)
            self.assertEqual(cell & 0x1FFF, 1)
            self.assertEqual((cell >> 13) & 7, bits)
        self.assertEqual(map_tool.cell_from_gid(0, 1), 0)
        # Tiled 的第 29 位也需清除，避免被誤當作瓦片 ID。
        self.assertEqual(map_tool.cell_from_gid(0x10000001, 1), 1)

    def test_stable_ids_and_reuse_when_only_flips_change(self):
        self.write_map()
        self.build()
        original_library = (self.output / "tileset.bin").read_bytes()
        self.write_map(values=[1, 2 | map_tool.TILED_V, 2 | map_tool.TILED_D, 0])
        map_tool.scan(self.input, self.output)
        map_tool.export_maps(self.input, self.output)
        self.assertEqual(original_library, (self.output / "tileset.bin").read_bytes())
        cells = struct.unpack_from("<4H", (self.output / "map.bin").read_bytes(), common.MAP_HEADER.size)
        self.assertEqual(cells, (1, 2 | map_tool.MAP_V, 2 | map_tool.MAP_D, 0))

    def test_new_tile_requires_library_rebuild(self):
        self.write_map(values=[1, 1, 0, 0])
        self.build()
        self.write_map(values=[1, 2, 0, 0])
        map_tool.scan(self.input, self.output)
        with self.assertRaisesRegex(common.ConversionError, "缺少"):
            map_tool.export_maps(self.input, self.output)
        tileset_tool.build_tileset(self.input, self.output)
        map_tool.export_maps(self.input, self.output)
        self.assertEqual(common.read_library(self.output)["ids"], [0, 1])

    def test_firstgid_and_sparse_mapping(self):
        self.write_map(first_gid=5, values=[6, 0, 0, 0])
        self.build()
        self.assertEqual(common.read_library(self.output)["ids"], [1])
        self.assertEqual(struct.unpack_from("<H", (self.output / "map.bin").read_bytes(), common.MAP_HEADER.size)[0], 2)

    def test_multiple_maps_share_original_ids_with_different_firstgids(self):
        self.write_map("first.tmx", first_gid=1, values=[1, 0, 0, 0])
        self.write_map("second.tmx", first_gid=5, values=[6, 0, 0, 0])
        self.build()
        library = common.read_library(self.output)
        self.assertEqual(library["ids"], [0, 1])
        first = (self.output / "first.bin").read_bytes()
        second = (self.output / "second.bin").read_bytes()
        self.assertEqual(common.MAP_HEADER.unpack_from(first)[-1], library["identifier"])
        self.assertEqual(common.MAP_HEADER.unpack_from(second)[-1], library["identifier"])
        self.assertEqual(struct.unpack_from("<H", first, common.MAP_HEADER.size)[0], 1)
        self.assertEqual(struct.unpack_from("<H", second, common.MAP_HEADER.size)[0], 2)

    def test_changed_map_requires_new_scan(self):
        self.write_map()
        map_tool.scan(self.input, self.output)
        tileset_tool.build_tileset(self.input, self.output)
        self.write_map(values=[1, 1, 1, 1])
        with self.assertRaisesRegex(common.ConversionError, "重新執行"):
            map_tool.export_maps(self.input, self.output)
        self.assertFalse((self.output / "map.bin").exists())

    def test_partial_alpha_threshold_and_count(self):
        self.write_map()
        map_tool.scan(self.input, self.output)
        for x, alpha in enumerate((0, 127, 128, 138, 255)):
            self.image.putpixel((x, 0), (255, 0, 0, alpha))
        self.save_image()
        original_png = (self.input / "tiles.png").read_bytes()
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            tileset_tool.build_tileset(self.input, self.output)
        library = common.read_library(self.output)
        self.assertEqual(library["palette"][:3], [0xf81f, 0x0000, 0xf800])
        self.assertEqual(library["pixels"][:5], b"\x00\x00\x02\x02\x02")
        self.assertIn("處理 3 個半透明像素", output.getvalue())
        self.assertEqual((self.input / "tiles.png").read_bytes(), original_png)

    def test_color_key_collision_after_alpha_threshold(self):
        self.write_map()
        map_tool.scan(self.input, self.output)
        self.image.putpixel((0, 0), (248, 0, 248, 127))
        self.save_image()
        tileset_tool.build_tileset(self.input, self.output)
        self.image.putpixel((0, 0), (248, 0, 248, 128))
        self.save_image()
        with self.assertRaisesRegex(common.ConversionError, "透明鍵衝突"):
            tileset_tool.build_tileset(self.input, self.output)

    def test_corrupt_or_old_version_library_rejected(self):
        self.write_map()
        self.build()
        path = self.output / "tileset.bin"
        data = bytearray(path.read_bytes())
        data[-1] ^= 1
        path.write_bytes(data)
        with self.assertRaisesRegex(common.ConversionError, "損壞"):
            map_tool.export_maps(self.input, self.output)
        data[-1] ^= 1
        data[4:6] = struct.pack("<H", 1)
        path.write_bytes(data)
        with self.assertRaisesRegex(common.ConversionError, "損壞"):
            map_tool.export_maps(self.input, self.output)

    def test_png_tool_rejects_outside_input(self):
        self.write_map()
        map_tool.scan(self.input, self.output)
        with self.assertRaisesRegex(common.ConversionError, "input"):
            tileset_tool.build_tileset(self.input, self.output, "../tiles.png")

    def test_rejects_invalid_png_grid_and_out_of_range_map(self):
        self.write_map(values=[3, 0, 0, 0])
        map_tool.scan(self.input, self.output)
        with self.assertRaisesRegex(common.ConversionError, "只有 2 片"):
            tileset_tool.build_tileset(self.input, self.output)
        Image.new("RGBA", (17, 16)).save(self.input / "tiles.png")
        with self.assertRaisesRegex(common.ConversionError, "16 的倍數"):
            tileset_tool.build_tileset(self.input, self.output)

    def test_separate_command_line_tools_end_to_end(self):
        self.write_map()
        work = self.root / "cli"
        work.mkdir()
        shutil.copytree(self.input, work / "input")
        for name in ("common.py", "map_tool.py", "tileset_tool.py"):
            shutil.copy2(Path(__file__).resolve().parent / name, work / name)
        for arguments in (("map_tool.py", "scan"), ("tileset_tool.py",), ("map_tool.py", "export")):
            result = subprocess.run([sys.executable, *arguments], cwd=work, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertTrue((work / "output" / "tileset.bin").is_file())
        self.assertTrue((work / "output" / "map.bin").is_file())


if __name__ == "__main__":
    unittest.main()
