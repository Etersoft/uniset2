"""Tests for st2js error classes."""
import pytest

from st2js.errors import (
    STError,
    ParseError,
    STTypeError,
    MappingError,
    UnsupportedError,
    STWarning,
)


class TestSTError:
    """Tests for the base STError exception class."""

    def test_str_with_all_fields(self):
        err = STError("something failed", file="prog.st", line=10, col=5)
        assert str(err) == "prog.st:10:5: error: something failed"

    def test_str_without_location(self):
        err = STError("something failed")
        assert str(err) == "error: something failed"

    def test_str_with_file_only(self):
        err = STError("something failed", file="prog.st")
        assert str(err) == "error: something failed"

    def test_str_with_file_and_line_no_col(self):
        err = STError("something failed", file="prog.st", line=10)
        assert str(err) == "error: something failed"

    def test_str_with_all_location_fields(self):
        err = STError("bad token", file="test.st", line=1, col=1)
        assert str(err) == "test.st:1:1: error: bad token"

    def test_is_exception(self):
        assert issubclass(STError, Exception)

    def test_fields_stored(self):
        err = STError("msg", file="f.st", line=3, col=7)
        assert err.message == "msg"
        assert err.file == "f.st"
        assert err.line == 3
        assert err.col == 7

    def test_fields_default_none(self):
        err = STError("msg")
        assert err.message == "msg"
        assert err.file is None
        assert err.line is None
        assert err.col is None


class TestParseError:
    """Tests for ParseError."""

    def test_is_subclass_of_sterror(self):
        assert issubclass(ParseError, STError)

    def test_str_with_location(self):
        err = ParseError("unexpected token", file="prog.st", line=5, col=12)
        assert str(err) == "prog.st:5:12: error: unexpected token"

    def test_str_without_location(self):
        err = ParseError("unexpected token")
        assert str(err) == "error: unexpected token"

    def test_can_be_caught_as_sterror(self):
        with pytest.raises(STError):
            raise ParseError("parse fail")


class TestSTTypeError:
    """Tests for STTypeError."""

    def test_is_subclass_of_sterror(self):
        assert issubclass(STTypeError, STError)

    def test_str_with_location(self):
        err = STTypeError("type mismatch", file="prog.st", line=20, col=3)
        assert str(err) == "prog.st:20:3: error: type mismatch"

    def test_str_without_location(self):
        err = STTypeError("type mismatch")
        assert str(err) == "error: type mismatch"

    def test_can_be_caught_as_sterror(self):
        with pytest.raises(STError):
            raise STTypeError("type fail")


class TestMappingError:
    """Tests for MappingError."""

    def test_is_subclass_of_sterror(self):
        assert issubclass(MappingError, STError)

    def test_str_with_location(self):
        err = MappingError("missing sensor", file="map.yaml", line=8, col=1)
        assert str(err) == "map.yaml:8:1: error: missing sensor"

    def test_str_without_location(self):
        err = MappingError("missing sensor")
        assert str(err) == "error: missing sensor"

    def test_can_be_caught_as_sterror(self):
        with pytest.raises(STError):
            raise MappingError("mapping fail")


class TestUnsupportedError:
    """Tests for UnsupportedError."""

    def test_is_subclass_of_sterror(self):
        assert issubclass(UnsupportedError, STError)

    def test_str_with_location(self):
        err = UnsupportedError("POINTER not supported", file="prog.st", line=15, col=1)
        assert str(err) == "prog.st:15:1: error: POINTER not supported"

    def test_str_without_location(self):
        err = UnsupportedError("POINTER not supported")
        assert str(err) == "error: POINTER not supported"

    def test_can_be_caught_as_sterror(self):
        with pytest.raises(STError):
            raise UnsupportedError("unsupported fail")


class TestSTWarning:
    """Tests for STWarning (not an exception)."""

    def test_is_not_exception(self):
        assert not issubclass(STWarning, Exception)

    def test_str_with_all_fields(self):
        warn = STWarning("unused variable", file="prog.st", line=10, col=5)
        assert str(warn) == "prog.st:10:5: warning: unused variable"

    def test_str_without_location(self):
        warn = STWarning("unused variable")
        assert str(warn) == "warning: unused variable"

    def test_str_with_partial_location(self):
        warn = STWarning("unused variable", file="prog.st")
        assert str(warn) == "warning: unused variable"

    def test_fields_stored(self):
        warn = STWarning("msg", file="f.st", line=3, col=7)
        assert warn.message == "msg"
        assert warn.file == "f.st"
        assert warn.line == 3
        assert warn.col == 7

    def test_fields_default_none(self):
        warn = STWarning("msg")
        assert warn.message == "msg"
        assert warn.file is None
        assert warn.line is None
        assert warn.col is None
