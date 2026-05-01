"""Integration tests using real ST/XML files from public projects.

These tests validate that our parser can handle real-world ST source code
from public repositories.  Files are stored in tests/fixtures/real_projects/.

Sources:
  - function-blocks.st, control-statements.st:
      https://github.com/jubnzv/iec-checker (test/st/good/)
  - codesys-small.xml:
      https://github.com/johannesPettersson80/trust-platform (PLCopen XML)
  - FB_Log.TcPOU:
      blark test fixtures (TwinCAT XML)
"""
from __future__ import annotations

import pathlib

import pytest

from st2js.errors import ParseError
from st2js.parser import ParseResult, parse_file, parse_st


REAL_DIR = pathlib.Path(__file__).parent / "fixtures" / "real_projects"


def _file_exists(name: str) -> bool:
    return (REAL_DIR / name).exists()


# ---------------------------------------------------------------------------
# iec-checker: function-blocks.st
# ---------------------------------------------------------------------------

@pytest.mark.skipif(
    not _file_exists("function-blocks.st"),
    reason="fixture function-blocks.st not available",
)
class TestIecCheckerFunctionBlocks:
    """Tests for iec-checker function-blocks.st (two FUNCTION_BLOCK defs)."""

    def test_parse_succeeds(self):
        result = parse_file(str(REAL_DIR / "function-blocks.st"))
        assert isinstance(result, ParseResult)
        assert result.tree is not None

    def test_contains_two_function_blocks(self):
        from blark.transform import FunctionBlock

        result = parse_file(str(REAL_DIR / "function-blocks.st"))
        fbs = [
            item for item in result.tree.items
            if isinstance(item, FunctionBlock)
        ]
        assert len(fbs) == 2

    def test_function_block_names(self):
        result = parse_file(str(REAL_DIR / "function-blocks.st"))
        names = [str(item.name) for item in result.tree.items]
        assert "fb0" in names
        assert "fb1" in names

    def test_fb1_has_variables(self):
        """fb1 should have VAR_INPUT, VAR_OUTPUT, VAR_IN_OUT, VAR, VAR_TEMP."""
        result = parse_file(str(REAL_DIR / "function-blocks.st"))
        fb1 = next(
            item for item in result.tree.items
            if str(item.name) == "fb1"
        )
        assert len(fb1.declarations) >= 5


# ---------------------------------------------------------------------------
# iec-checker: control-statements.st
# ---------------------------------------------------------------------------

@pytest.mark.skipif(
    not _file_exists("control-statements.st"),
    reason="fixture control-statements.st not available",
)
class TestIecCheckerControlStatements:
    """Tests for iec-checker control-statements.st (multiple PROGRAMs + FUNCTION)."""

    def test_parse_succeeds(self):
        result = parse_file(str(REAL_DIR / "control-statements.st"))
        assert isinstance(result, ParseResult)
        assert result.tree is not None

    def test_contains_five_items(self):
        """File has 4 PROGRAMs and 1 FUNCTION."""
        result = parse_file(str(REAL_DIR / "control-statements.st"))
        assert len(result.tree.items) == 5

    def test_program_names(self):
        result = parse_file(str(REAL_DIR / "control-statements.st"))
        names = [str(item.name) for item in result.tree.items]
        assert "square_root" in names
        assert "test_for" in names
        assert "test_switch_case" in names
        assert "p0" in names

    def test_contains_function(self):
        from blark.transform import Function

        result = parse_file(str(REAL_DIR / "control-statements.st"))
        functions = [
            item for item in result.tree.items
            if isinstance(item, Function)
        ]
        assert len(functions) == 1
        assert str(functions[0].name) == "fn0"

    def test_square_root_has_if_statement(self):
        from blark.transform import IfStatement

        result = parse_file(str(REAL_DIR / "control-statements.st"))
        prog = next(
            item for item in result.tree.items
            if str(item.name) == "square_root"
        )
        # Body should contain assignments and an IF statement
        has_if = any(
            isinstance(stmt, IfStatement)
            for stmt in prog.body.statements
        )
        assert has_if

    def test_p0_has_for_while_repeat(self):
        """Program p0 should contain FOR, WHILE, and REPEAT statements."""
        from blark.transform import ForStatement, RepeatStatement, WhileStatement

        result = parse_file(str(REAL_DIR / "control-statements.st"))
        prog = next(
            item for item in result.tree.items
            if str(item.name) == "p0"
        )
        stmt_types = {type(s) for s in prog.body.statements}
        assert ForStatement in stmt_types
        assert WhileStatement in stmt_types
        assert RepeatStatement in stmt_types


# ---------------------------------------------------------------------------
# PLCopen XML: codesys-small.xml
# ---------------------------------------------------------------------------

@pytest.mark.skipif(
    not _file_exists("codesys-small.xml"),
    reason="fixture codesys-small.xml not available",
)
class TestCodesysSmallXML:
    """Tests for PLCopen XML file from Codesys (codesys-small.xml)."""

    def test_parse_succeeds(self):
        result = parse_file(str(REAL_DIR / "codesys-small.xml"))
        assert isinstance(result, ParseResult)
        assert result.tree is not None

    def test_contains_program_main(self):
        result = parse_file(str(REAL_DIR / "codesys-small.xml"))
        names = [str(item.name) for item in result.tree.items]
        assert "Main" in names

    def test_program_has_variable(self):
        """Main program should declare the 'Started' variable."""
        result = parse_file(str(REAL_DIR / "codesys-small.xml"))
        program = result.tree.items[0]
        var_names = []
        for decl in program.declarations:
            for item in decl.items:
                for dv in item.variables:
                    var_names.append(str(dv.variable.name))
        assert "Started" in var_names

    def test_filename_set(self):
        path = str(REAL_DIR / "codesys-small.xml")
        result = parse_file(path)
        assert result.filename == path


# ---------------------------------------------------------------------------
# TwinCAT XML: FB_Log.TcPOU
# ---------------------------------------------------------------------------

@pytest.mark.skipif(
    not _file_exists("FB_Log.TcPOU"),
    reason="fixture FB_Log.TcPOU not available",
)
class TestFBLogTcPOU:
    """Tests for TwinCAT FB_Log.TcPOU (complex real-world function block)."""

    def test_parse_succeeds(self):
        result = parse_file(str(REAL_DIR / "FB_Log.TcPOU"))
        assert isinstance(result, ParseResult)
        assert result.tree is not None

    def test_contains_function_block(self):
        from blark.transform import FunctionBlock

        result = parse_file(str(REAL_DIR / "FB_Log.TcPOU"))
        fbs = [
            item for item in result.tree.items
            if isinstance(item, FunctionBlock)
        ]
        assert len(fbs) >= 1

    def test_fb_log_name(self):
        from blark.transform import FunctionBlock

        result = parse_file(str(REAL_DIR / "FB_Log.TcPOU"))
        fb_names = [
            str(item.name)
            for item in result.tree.items
            if isinstance(item, FunctionBlock)
        ]
        assert "FB_Log" in fb_names

    def test_has_methods(self):
        """FB_Log should contain methods (parsed as separate items)."""
        from blark.transform import Method

        result = parse_file(str(REAL_DIR / "FB_Log.TcPOU"))
        methods = [
            item for item in result.tree.items
            if isinstance(item, Method)
        ]
        assert len(methods) >= 1

    def test_many_items_merged(self):
        """TcPOU with methods should produce many merged items."""
        result = parse_file(str(REAL_DIR / "FB_Log.TcPOU"))
        # FB_Log has the FB itself + multiple methods + properties
        assert len(result.tree.items) >= 5


# ---------------------------------------------------------------------------
# Cross-format: same content parsed from ST vs XML should match
# ---------------------------------------------------------------------------

class TestCrossFormatConsistency:
    """Verify parse_file produces consistent results across formats."""

    @pytest.mark.skipif(
        not _file_exists("codesys-small.xml"),
        reason="fixture codesys-small.xml not available",
    )
    def test_xml_and_inline_st_produce_same_program(self):
        """The embedded ST in codesys-small.xml should match direct parse."""
        # Parse from XML
        xml_result = parse_file(str(REAL_DIR / "codesys-small.xml"))
        xml_prog = xml_result.tree.items[0]

        # Parse the same ST directly
        st_source = (
            "PROGRAM Main\n"
            "VAR\n"
            "    Started : BOOL := TRUE;\n"
            "END_VAR\n"
            "END_PROGRAM\n"
        )
        st_result = parse_st(st_source)
        st_prog = st_result.tree.items[0]

        assert str(xml_prog.name) == str(st_prog.name)
        assert len(xml_prog.declarations) == len(st_prog.declarations)
