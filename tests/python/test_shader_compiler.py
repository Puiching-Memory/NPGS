import importlib.util
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SHADER_COMPILER_PATH = REPO_ROOT / "NPGS" / "Tools" / "ShaderCompiler" / "CompileShaders.py"

spec = importlib.util.spec_from_file_location("npgs_shader_compiler", SHADER_COMPILER_PATH)
shader_compiler = importlib.util.module_from_spec(spec)
spec.loader.exec_module(shader_compiler)


class ShaderCompilerTests(unittest.TestCase):
    def test_config_parser_reads_variants_and_macros(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            config = Path(temp_dir) / "CompileShaders.cfg"
            config.write_text(
                "// comment\nScene.frag.glsl Scene.spv FIRST SECOND=2\n",
                encoding="utf-8",
            )

            variants = shader_compiler.load_shader_config(config)

            self.assertEqual(1, len(variants))
            self.assertEqual("Scene.frag.glsl", variants[0].input_path)
            self.assertEqual("Scene.spv", variants[0].output_name)
            self.assertEqual(["FIRST", "SECOND=2"], variants[0].macros)

    def test_recursive_includes_are_discovered(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            source = root / "source.glsl"
            common = root / "common.glsl"
            nested = root / "nested.glsl"
            source.write_text('#include "common.glsl"\n', encoding="utf-8")
            common.write_text('#include "nested.glsl"\n', encoding="utf-8")
            nested.write_text("const float value = 1.0;\n", encoding="utf-8")

            dependencies = shader_compiler.parse_includes(source)

            self.assertEqual({common.resolve(), nested.resolve()}, dependencies)

    def test_dependency_content_changes_hash(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            source = root / "source.glsl"
            dependency = root / "common.glsl"
            source.write_text("void main() {}\n", encoding="utf-8")
            dependency.write_text("const int value = 1;\n", encoding="utf-8")
            first_hash = shader_compiler.get_shader_hash(source, ["TEST"], {dependency})

            dependency.write_text("const int value = 2;\n", encoding="utf-8")
            second_hash = shader_compiler.get_shader_hash(source, ["TEST"], {dependency})

            self.assertNotEqual(first_hash, second_hash)


if __name__ == "__main__":
    unittest.main()
