"""Tests for the st2js parser module.

Validates blark integration, ParseResult structure, error handling,
and parsing of 5+ representative ST constructs.
"""
import pathlib

import pytest

from st2js.errors import ParseError
from st2js.parser import ParseResult, parse_file, parse_st


FIXTURES_DIR = pathlib.Path(__file__).parent / "fixtures"
REAL_PROJECTS_DIR = FIXTURES_DIR / "real_projects"


# ---------------------------------------------------------------------------
# Helper to load fixture files
# ---------------------------------------------------------------------------

def load_fixture(name: str) -> str:
    """Load a .st fixture file and return its contents."""
    return (FIXTURES_DIR / name).read_text()


# ---------------------------------------------------------------------------
# Test: minimal.st parses successfully
# ---------------------------------------------------------------------------

class TestMinimalST:
    """Tests for the minimal.st fixture file."""

    def test_parses_successfully(self):
        source = load_fixture("minimal.st")
        result = parse_st(source, filename="minimal.st")
        assert isinstance(result, ParseResult)
        assert result.source == source
        assert result.filename == "minimal.st"
        assert result.tree is not None

    def test_program_name(self):
        source = load_fixture("minimal.st")
        result = parse_st(source, filename="minimal.st")
        programs = result.tree.items
        assert len(programs) == 1
        program = programs[0]
        assert str(program.name) == "Main"

    def test_variable_declarations(self):
        """Verify that parsed AST contains expected variable sections."""
        source = load_fixture("minimal.st")
        result = parse_st(source, filename="minimal.st")
        program = result.tree.items[0]

        from blark.transform import (
            InputDeclarations,
            OutputDeclarations,
            VariableDeclarations,
        )

        decl_types = [type(d) for d in program.declarations]
        assert InputDeclarations in decl_types
        assert OutputDeclarations in decl_types
        assert VariableDeclarations in decl_types

    def test_input_variable_names(self):
        """Verify input variables are Temperature and StartButton."""
        source = load_fixture("minimal.st")
        result = parse_st(source, filename="minimal.st")
        program = result.tree.items[0]

        from blark.transform import InputDeclarations

        input_decl = next(
            d for d in program.declarations
            if isinstance(d, InputDeclarations)
        )
        var_names = []
        for item in input_decl.items:
            for dv in item.variables:
                var_names.append(str(dv.variable.name))
        assert var_names == ["Temperature", "StartButton"]

    def test_body_has_statements(self):
        """Verify that the program body contains IF statements."""
        source = load_fixture("minimal.st")
        result = parse_st(source, filename="minimal.st")
        program = result.tree.items[0]
        assert program.body is not None
        assert len(program.body.statements) == 2


# ---------------------------------------------------------------------------
# Test: invalid ST raises ParseError
# ---------------------------------------------------------------------------

class TestInvalidST:
    """Tests that invalid ST source raises ParseError with location info."""

    def test_garbage_raises_parse_error(self):
        with pytest.raises(ParseError) as exc_info:
            parse_st("THIS IS NOT VALID ST CODE !!!")
        err = exc_info.value
        assert err.file == "<stdin>"
        assert err.line is not None
        assert err.col is not None

    def test_unclosed_if_raises_parse_error(self):
        source = (
            "PROGRAM Bad\n"
            "VAR\n"
            "    x : INT;\n"
            "END_VAR\n"
            "IF x > 0 THEN\n"
            "    x := 1;\n"
            "END_PROGRAM\n"
        )
        with pytest.raises(ParseError) as exc_info:
            parse_st(source, filename="bad.st")
        err = exc_info.value
        assert err.file == "bad.st"
        assert "bad.st" in str(err)

    def test_missing_end_program_raises_parse_error(self):
        source = (
            "PROGRAM Bad\n"
            "VAR\n"
            "    x : INT;\n"
            "END_VAR\n"
            "x := 1;\n"
        )
        with pytest.raises(ParseError):
            parse_st(source, filename="incomplete.st")

    def test_parse_error_format_includes_line_col(self):
        """Verify ParseError string includes file:line:col format."""
        with pytest.raises(ParseError) as exc_info:
            parse_st("THIS IS NOT VALID ST CODE !!!", filename="test.st")
        err_str = str(exc_info.value)
        assert "test.st" in err_str
        assert "error:" in err_str


# ---------------------------------------------------------------------------
# Test: 5+ ST constructs parse successfully (risk validation)
# ---------------------------------------------------------------------------

class TestSTConstructs:
    """Validate blark parses 5+ representative ST constructs.

    This is the critical risk validation for blark compatibility.
    """

    def test_simple_program(self):
        """Construct 1: Simple PROGRAM with variables and assignments."""
        source = """
PROGRAM SimpleTest
VAR
    x : INT := 0;
    y : REAL;
END_VAR

x := 42;
y := 3.14;

END_PROGRAM
"""
        result = parse_st(source)
        program = result.tree.items[0]
        assert str(program.name) == "SimpleTest"
        assert len(program.body.statements) == 2

    def test_function_block(self):
        """Construct 2: FUNCTION_BLOCK declaration."""
        source = """
FUNCTION_BLOCK MyCounter
VAR_INPUT
    Enable : BOOL;
    Reset : BOOL;
END_VAR
VAR_OUTPUT
    Count : INT;
END_VAR
VAR
    counter : INT := 0;
END_VAR

IF Reset THEN
    counter := 0;
ELSIF Enable THEN
    counter := counter + 1;
END_IF;

Count := counter;

END_FUNCTION_BLOCK
"""
        result = parse_st(source)
        fb = result.tree.items[0]

        from blark.transform import FunctionBlock
        assert isinstance(fb, FunctionBlock)
        assert str(fb.name) == "MyCounter"

    def test_case_statement(self):
        """Construct 3: CASE..OF with multiple branches and ELSE."""
        source = """
PROGRAM CaseTest
VAR
    state : INT := 0;
    value : INT;
END_VAR

CASE state OF
    0: value := 10;
    1: value := 20;
    2, 3: value := 30;
ELSE
    value := 0;
END_CASE;

END_PROGRAM
"""
        result = parse_st(source)
        program = result.tree.items[0]
        assert str(program.name) == "CaseTest"

        from blark.transform import CaseStatement
        case_stmt = program.body.statements[0]
        assert isinstance(case_stmt, CaseStatement)

    def test_for_loop(self):
        """Construct 4: FOR loop with BY step."""
        source = """
PROGRAM ForTest
VAR
    i : INT;
    total : INT := 0;
END_VAR

FOR i := 1 TO 10 BY 2 DO
    total := total + i;
END_FOR;

END_PROGRAM
"""
        result = parse_st(source)
        program = result.tree.items[0]

        from blark.transform import ForStatement
        for_stmt = program.body.statements[0]
        assert isinstance(for_stmt, ForStatement)

    def test_while_loop(self):
        """Construct 5: WHILE loop."""
        source = """
PROGRAM WhileTest
VAR
    count : INT := 0;
END_VAR

WHILE count < 100 DO
    count := count + 1;
END_WHILE;

END_PROGRAM
"""
        result = parse_st(source)
        program = result.tree.items[0]

        from blark.transform import WhileStatement
        while_stmt = program.body.statements[0]
        assert isinstance(while_stmt, WhileStatement)

    def test_fb_instantiation_and_call(self):
        """Construct 6: FB instantiation and method call with named args."""
        source = """
PROGRAM FBTest
VAR
    myTimer : TON;
    timerDone : BOOL;
END_VAR

myTimer(IN := TRUE, PT := T#5s);
timerDone := myTimer.Q;

END_PROGRAM
"""
        result = parse_st(source)
        program = result.tree.items[0]
        assert len(program.body.statements) == 2

    def test_repeat_until(self):
        """Construct 7: REPEAT..UNTIL loop."""
        source = """
PROGRAM RepeatTest
VAR
    x : INT := 0;
END_VAR

REPEAT
    x := x + 1;
UNTIL x >= 10
END_REPEAT;

END_PROGRAM
"""
        result = parse_st(source)
        program = result.tree.items[0]

        from blark.transform import RepeatStatement
        repeat_stmt = program.body.statements[0]
        assert isinstance(repeat_stmt, RepeatStatement)


# ---------------------------------------------------------------------------
# Test: default filename
# ---------------------------------------------------------------------------

class TestDefaults:
    """Test default parameter values."""

    def test_default_filename(self):
        source = """
PROGRAM Minimal
VAR
    x : INT;
END_VAR
x := 1;
END_PROGRAM
"""
        result = parse_st(source)
        assert result.filename == "<stdin>"


# ---------------------------------------------------------------------------
# Test: AST structure debugging output (prints for debugging)
# ---------------------------------------------------------------------------

class TestASTDebugOutput:
    """Print blark AST structure for debugging purposes.

    These tests always pass but produce useful debugging output
    showing the AST node types for the transformer implementation.
    """

    def test_print_ast_structure(self, capsys):
        """Print the full AST structure of minimal.st for debugging."""
        source = load_fixture("minimal.st")
        result = parse_st(source, filename="minimal.st")
        program = result.tree.items[0]

        print("\n=== BLARK AST STRUCTURE FOR minimal.st ===")
        print(f"Program name: {program.name}")
        print(f"Program type: {type(program).__name__}")
        print(f"Declarations count: {len(program.declarations)}")

        for i, decl in enumerate(program.declarations):
            print(f"\n  Declaration {i}: {type(decl).__name__}")
            for j, item in enumerate(decl.items):
                print(f"    Item {j}: {type(item).__name__}")
                for k, var in enumerate(item.variables):
                    print(f"      Variable {k}: {type(var).__name__} "
                          f"name={var.variable.name}")
                print(f"    Init: {type(item.init).__name__} "
                      f"spec={item.init.spec} value={item.init.value}")

        print(f"\nBody type: {type(program.body).__name__}")
        print(f"Body statements count: {len(program.body.statements)}")
        for i, stmt in enumerate(program.body.statements):
            print(f"  Statement {i}: {type(stmt).__name__}")
            _print_stmt_tree(stmt, indent=4)

        captured = capsys.readouterr()
        assert "BLARK AST STRUCTURE" in captured.out


def _print_stmt_tree(node, indent=0):
    """Recursively print AST node structure for debugging."""
    prefix = " " * indent
    attrs = [a for a in dir(node) if not a.startswith("_")
             and a not in ("from_lark", "meta")]
    for attr_name in attrs:
        try:
            val = getattr(node, attr_name)
            if callable(val):
                continue
            print(f"{prefix}{attr_name}: {type(val).__name__} = {repr(val)[:120]}")
        except Exception:
            pass


# ---------------------------------------------------------------------------
# Test: parse_file() with plain ST files
# ---------------------------------------------------------------------------

class TestParseFileST:
    """Tests for parse_file() with plain ST input files."""

    def test_parses_st_file(self):
        """parse_file() should handle .st files via parse_st()."""
        path = str(FIXTURES_DIR / "minimal.st")
        result = parse_file(path)
        assert isinstance(result, ParseResult)
        assert result.tree is not None
        program = result.tree.items[0]
        assert str(program.name) == "Main"

    def test_filename_is_set(self):
        """parse_file() should set filename to the input path."""
        path = str(FIXTURES_DIR / "minimal.st")
        result = parse_file(path)
        assert result.filename == path

    def test_nonexistent_file_raises_parse_error(self):
        """parse_file() should raise ParseError for missing files."""
        with pytest.raises(ParseError, match="file not found"):
            parse_file("/nonexistent/path/to/file.st")

    def test_thermostat_st(self):
        """parse_file() should handle thermostat.st."""
        path = str(FIXTURES_DIR / "thermostat.st")
        result = parse_file(path)
        assert result.tree is not None
        assert len(result.tree.items) >= 1


# ---------------------------------------------------------------------------
# Test: parse_file() with TwinCAT XML files
# ---------------------------------------------------------------------------

TCPOU_FILE = FIXTURES_DIR / "real_projects" / "FB_Log.TcPOU"


class TestParseFileXML:
    """Tests for parse_file() with TwinCAT XML input files."""

    @pytest.mark.skipif(
        not TCPOU_FILE.exists(),
        reason="blark test fixture FB_Log.TcPOU not available",
    )
    def test_parses_tcpou_file(self):
        """parse_file() should parse a TwinCAT .TcPOU file."""
        result = parse_file(str(TCPOU_FILE))
        assert isinstance(result, ParseResult)
        assert result.tree is not None
        assert len(result.tree.items) >= 1

    @pytest.mark.skipif(
        not TCPOU_FILE.exists(),
        reason="blark test fixture FB_Log.TcPOU not available",
    )
    def test_tcpou_contains_function_block(self):
        """FB_Log.TcPOU should contain a FunctionBlock named FB_Log."""
        from blark.transform import FunctionBlock

        result = parse_file(str(TCPOU_FILE))
        fb_names = [
            str(item.name)
            for item in result.tree.items
            if isinstance(item, FunctionBlock)
        ]
        assert "FB_Log" in fb_names

    @pytest.mark.skipif(
        not TCPOU_FILE.exists(),
        reason="blark test fixture FB_Log.TcPOU not available",
    )
    def test_tcpou_filename_set(self):
        """parse_file() should set filename for XML files."""
        result = parse_file(str(TCPOU_FILE))
        assert result.filename == str(TCPOU_FILE)

    def test_nonexistent_xml_raises_parse_error(self):
        """parse_file() should raise ParseError for missing XML files."""
        with pytest.raises(ParseError):
            parse_file("/nonexistent/path/to/file.TcPOU")
