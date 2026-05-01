#!/bin/sh
# Run st2js pytest suite
cd "$(dirname "$0")"
exec python3 -m pytest tests/ -q --tb=short
