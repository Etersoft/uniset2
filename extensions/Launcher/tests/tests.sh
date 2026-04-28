#!/bin/sh

# Run the full Launcher test suite (Catch2 unit tests + shell regressions).
# Delegates to `make check` so anyone running this entry script gets the
# same coverage as the autotools target.
exec make check $*
