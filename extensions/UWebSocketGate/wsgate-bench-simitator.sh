#!/usr/bin/env bash
set -euo pipefail

./ws-client.py --ws ws://localhost:8081/wsgate --start-id 10001 --count 5000 --duration 30 --check-responses
