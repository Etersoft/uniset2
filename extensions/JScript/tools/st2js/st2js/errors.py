#!/usr/bin/env python3
"""Error and warning classes for the st2js converter.

All error classes support optional source location (file, line, col) and format
messages as: {file}:{line}:{col}: {type}: {message}
"""
from typing import Optional


class STError(Exception):
    """Base error class for all st2js conversion errors.

    Formats as '{file}:{line}:{col}: error: {message}' when location is available,
    or 'error: {message}' when location is not set.
    """

    def __init__(
        self,
        message: str,
        file: Optional[str] = None,
        line: Optional[int] = None,
        col: Optional[int] = None,
    ):
        self.message = message
        self.file = file
        self.line = line
        self.col = col
        super().__init__(str(self))

    def _format(self, level: str) -> str:
        if self.file is not None and self.line is not None and self.col is not None:
            return f"{self.file}:{self.line}:{self.col}: {level}: {self.message}"
        return f"{level}: {self.message}"

    def __str__(self) -> str:
        return self._format("error")


class ParseError(STError):
    """Raised when ST source cannot be parsed."""


class STTypeError(STError):
    """Raised on type checking failures."""


class MappingError(STError):
    """Raised on sensor mapping errors."""


class UnsupportedError(STError):
    """Raised for unsupported ST constructs."""


class STWarning:
    """Non-exception warning with source location.

    Formats as '{file}:{line}:{col}: warning: {message}' when location is available,
    or 'warning: {message}' when location is not set.
    """

    def __init__(
        self,
        message: str,
        file: Optional[str] = None,
        line: Optional[int] = None,
        col: Optional[int] = None,
    ):
        self.message = message
        self.file = file
        self.line = line
        self.col = col

    def __str__(self) -> str:
        if self.file is not None and self.line is not None and self.col is not None:
            return f"{self.file}:{self.line}:{self.col}: warning: {self.message}"
        return f"warning: {self.message}"
