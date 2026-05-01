"""Tests for the st2js CLI (cli.py).

Tests cover:
- main() function returns 0 on success
- main() reads ST file, applies mapping, generates JS to stdout
- main() writes to file with -o flag
- --strict flag accepted
- --struct-flatten flag accepted
- --version flag prints version
- Non-zero exit on parse error
- Non-zero exit on missing mapping file
- Non-zero exit on missing input file
"""
from __future__ import annotations

import os
import subprocess
import sys
import tempfile

import pytest


FIXTURES_DIR = os.path.join(os.path.dirname(__file__), "fixtures")
MINIMAL_ST = os.path.join(FIXTURES_DIR, "minimal.st")
MINIMAL_MAPPING = os.path.join(FIXTURES_DIR, "minimal_mapping.yaml")
# Package root where 'st2js/' directory lives (needed for python -m st2js)
PACKAGE_ROOT = os.path.dirname(os.path.dirname(__file__))


class TestMainFunction:
    """Test cli.main() function directly."""

    def test_main_returns_int(self):
        from st2js.cli import main

        # No args prints help and returns 0
        result = main([])
        assert isinstance(result, int)

    def test_main_no_args_returns_zero(self):
        from st2js.cli import main

        result = main([])
        assert result == 0

    def test_main_success_with_valid_input(self):
        from st2js.cli import main

        with tempfile.NamedTemporaryFile(suffix=".js", delete=False) as f:
            outpath = f.name

        try:
            result = main([MINIMAL_ST, "-m", MINIMAL_MAPPING, "-o", outpath])
            assert result == 0

            with open(outpath) as f:
                js = f.read()

            assert 'load("uniset2-iec61131.js");' in js
            assert "uniset_inputs" in js
            assert "uniset_outputs" in js
            assert "function uniset_on_step()" in js
        finally:
            os.unlink(outpath)

    def test_main_outputs_to_stdout(self, capsys):
        from st2js.cli import main

        result = main([MINIMAL_ST, "-m", MINIMAL_MAPPING])
        assert result == 0

        captured = capsys.readouterr()
        assert 'load("uniset2-iec61131.js");' in captured.out
        assert "function uniset_on_step()" in captured.out

    def test_main_strict_flag_accepted(self):
        from st2js.cli import main

        with tempfile.NamedTemporaryFile(suffix=".js", delete=False) as f:
            outpath = f.name

        try:
            result = main([MINIMAL_ST, "-m", MINIMAL_MAPPING, "-o", outpath, "--strict"])
            assert result == 0
        finally:
            os.unlink(outpath)

    def test_main_struct_flatten_flag_accepted(self):
        from st2js.cli import main

        with tempfile.NamedTemporaryFile(suffix=".js", delete=False) as f:
            outpath = f.name

        try:
            result = main([MINIMAL_ST, "-m", MINIMAL_MAPPING, "-o", outpath, "--struct-flatten"])
            assert result == 0
        finally:
            os.unlink(outpath)


class TestMainErrorHandling:
    """Test error handling in cli.main()."""

    def test_missing_input_file_returns_nonzero(self):
        from st2js.cli import main

        result = main(["nonexistent.st", "-m", MINIMAL_MAPPING])
        assert result != 0

    def test_missing_mapping_file_returns_nonzero(self):
        from st2js.cli import main

        result = main([MINIMAL_ST, "-m", "nonexistent_mapping.yaml"])
        assert result != 0

    def test_invalid_st_returns_nonzero(self):
        from st2js.cli import main

        with tempfile.NamedTemporaryFile(
            suffix=".st", mode="w", delete=False
        ) as f:
            f.write("THIS IS NOT VALID ST CODE !!!")
            invalid_st = f.name

        try:
            result = main([invalid_st, "-m", MINIMAL_MAPPING])
            assert result != 0
        finally:
            os.unlink(invalid_st)

    def test_error_prints_to_stderr(self, capsys):
        from st2js.cli import main

        main(["nonexistent.st", "-m", MINIMAL_MAPPING])

        captured = capsys.readouterr()
        assert captured.err != ""

    def test_no_mapping_uses_st_variable_names(self):
        """Input provided without -m flag — uses ST variable names as sensor names."""
        from st2js.cli import main

        result = main([MINIMAL_ST])
        assert result == 0


class TestCLISubprocess:
    """Test CLI via subprocess (python -m st2js)."""

    def test_version_flag(self):
        result = subprocess.run(
            [sys.executable, "-m", "st2js", "--version"],
            capture_output=True,
            text=True,
            cwd=PACKAGE_ROOT,
        )
        assert result.returncode == 0
        assert "0.1.0" in result.stdout

    def test_help_flag(self):
        result = subprocess.run(
            [sys.executable, "-m", "st2js", "--help"],
            capture_output=True,
            text=True,
            cwd=PACKAGE_ROOT,
        )
        assert result.returncode == 0
        assert "st2js" in result.stdout

    def test_full_pipeline_subprocess(self):
        result = subprocess.run(
            [
                sys.executable, "-m", "st2js",
                MINIMAL_ST,
                "-m", MINIMAL_MAPPING,
            ],
            capture_output=True,
            text=True,
        )
        assert result.returncode == 0
        assert 'load("uniset2-iec61131.js");' in result.stdout
        assert "function uniset_on_step()" in result.stdout

    def test_invalid_file_subprocess(self):
        result = subprocess.run(
            [
                sys.executable, "-m", "st2js",
                "nonexistent.st",
                "-m", MINIMAL_MAPPING,
            ],
            capture_output=True,
            text=True,
        )
        assert result.returncode != 0
        assert result.stderr != ""


class TestLoadDirectiveFlags:
    """Tests for --load-head / --load-on-start CLI flags."""

    def test_load_head_flag_emits_load(self, tmp_path):
        import shutil
        st_file = tmp_path / "min.st"
        shutil.copy(MINIMAL_ST, st_file)
        out_file = tmp_path / "out.js"

        from st2js.cli import main
        rc = main([
            str(st_file),
            "-m", MINIMAL_MAPPING,
            "-o", str(out_file),
            "--load-head", "cli_head.js",
        ])
        assert rc == 0
        out = out_file.read_text()
        assert 'load("cli_head.js");' in out

    def test_load_on_start_flag_emits_load(self, tmp_path):
        import shutil
        st_file = tmp_path / "min.st"
        shutil.copy(MINIMAL_ST, st_file)
        out_file = tmp_path / "out.js"

        from st2js.cli import main
        rc = main([
            str(st_file),
            "-m", MINIMAL_MAPPING,
            "-o", str(out_file),
            "--load-on-start", "cli_sim.js",
        ])
        assert rc == 0
        out = out_file.read_text()
        assert 'load("cli_sim.js");' in out
        assert "function uniset_on_start() {" in out

    def test_cli_and_yaml_merge_dedup(self, tmp_path):
        import shutil
        st_file = tmp_path / "min.st"
        shutil.copy(MINIMAL_ST, st_file)
        yaml_file = tmp_path / "map.yaml"
        yaml_file.write_text(
            "load_on_start:\n  - yaml_sim.js\n  - shared.js\n"
        )
        out_file = tmp_path / "out.js"

        from st2js.cli import main
        rc = main([
            str(st_file),
            "-m", str(yaml_file),
            "-o", str(out_file),
            "--load-on-start", "shared.js",
            "--load-on-start", "cli_sim.js",
        ])
        assert rc == 0
        out = out_file.read_text()
        start_idx = out.index("function uniset_on_start() {")
        body = out[start_idx:out.index("}", start_idx) + 1]
        assert body.count("load(") == 3
        assert body.index("yaml_sim.js") < body.index("shared.js")
        assert body.index("shared.js") < body.index("cli_sim.js")

    def test_multi_file_mode_preserves_load_on_start(self, tmp_path):
        """I1: multi-file mode must include load_on_start in the combined output."""
        import shutil
        st1 = tmp_path / "a.st"
        st2 = tmp_path / "b.st"
        shutil.copy(MINIMAL_ST, st1)
        shutil.copy(MINIMAL_ST, st2)
        mapping = tmp_path / "map.yaml"
        mapping.write_text(
            "load_on_start:\n"
            "  - sim_shared.js\n"
        )
        out = tmp_path / "out.js"

        from st2js.cli import main
        rc = main([
            str(st1), str(st2),
            "-m", str(mapping),
            "-o", str(out),
        ])
        assert rc == 0, "multi-file pipeline failed"

        js = out.read_text()
        # Must appear exactly once inside uniset_on_start body
        start_idx = js.index("function uniset_on_start() {")
        close_idx = js.index("}", start_idx)
        body = js[start_idx:close_idx + 1]
        assert body.count('load("sim_shared.js");') == 1, \
            f"expected one load() inside uniset_on_start, got body:\n{body}"
