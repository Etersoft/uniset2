#!/bin/bash
set -e

CONFFILE="${CONFFILE:-/app/configure.xml}"
NODE_NAME="${NODE_NAME:-Tester}"

echo "=== E2E Test Suite ==="
echo "Config: $CONFFILE"
echo "Node: $NODE_NAME"
echo ""

# Wait for nodes - UNet needs significant time to establish connections
echo "Waiting for nodes to start..."
sleep 30

# Wait for SharedMemory on Node1 and Node2 to be ready
wait_for_sm() {
    local node=$1
    local max_attempts=30
    local attempt=0

    echo "Waiting for SharedMemory on $node..."
    while [ $attempt -lt $max_attempts ]; do
        if uniset2-admin --confile "$CONFFILE" --getValue TestMode_S@$node 2>&1 | grep -q "value:"; then
            echo "  $node: SharedMemory ready"
            return 0
        fi
        attempt=$((attempt + 1))
        sleep 1
    done
    echo "  $node: TIMEOUT waiting for SharedMemory"
    return 1
}

wait_for_sm Node1
wait_for_sm Node2

# Wait for UNet synchronization to be established
# Note: UNet checkConnectionPause defaults to 10 seconds, so receivers take time to connect
wait_for_unet() {
    local max_attempts=60
    local attempt=0

    echo "Waiting for UNet synchronization..."
    while [ $attempt -lt $max_attempts ]; do
        # Check if Node1 responds (seen by Node2) and Node2 responds (seen by Node1)
        local node1_resp=$(uniset2-admin --confile "$CONFFILE" --getValue Node1_Respond_S@Node2 2>&1 | grep "value:" | awk '{print $2}')
        local node2_resp=$(uniset2-admin --confile "$CONFFILE" --getValue Node2_Respond_S@Node1 2>&1 | grep "value:" | awk '{print $2}')

        if [ "$node1_resp" = "1" ] && [ "$node2_resp" = "1" ]; then
            echo "  UNet sync established (Node1_Respond=$node1_resp, Node2_Respond=$node2_resp)"
            return 0
        fi
        echo "  Waiting... (Node1_Respond=$node1_resp, Node2_Respond=$node2_resp)"
        attempt=$((attempt + 1))
        sleep 1
    done
    echo "  WARNING: UNet sync not fully established"
    return 0  # Continue anyway
}

wait_for_unet

echo ""

# Test counters
PASSED=0
FAILED=0

# Helper function
run_test() {
    local name="$1"
    local cmd="$2"

    echo "--- Test: $name ---"
    if eval "$cmd"; then
        echo "[PASS] $name"
        PASSED=$((PASSED + 1))
        return 0
    else
        echo "[FAIL] $name"
        FAILED=$((FAILED + 1))
        return 1
    fi
}

# ============== Test 1: SM Exchange via UNet ==============
test_unet_exchange() {
    local TEST_VALUE=$((RANDOM % 10000 + 1000))

    echo "Setting TestValue1_AS@Node1 = $TEST_VALUE"
    uniset2-admin --confile "$CONFFILE" --setValue TestValue1_AS@Node1=$TEST_VALUE

    echo "Waiting 3 seconds for UNet sync..."
    sleep 3

    echo "Reading TestValue1_AS@Node2..."
    local RESULT=$(uniset2-admin --confile "$CONFFILE" --getValue TestValue1_AS@Node2 2>&1 | grep "value:" | awk '{print $2}')
    echo "Got value: $RESULT"

    if [ "$RESULT" = "$TEST_VALUE" ]; then
        echo "Value propagated correctly via UNet"
        return 0
    else
        echo "ERROR: Expected $TEST_VALUE, got $RESULT"
        return 1
    fi
}

# ============== Test 2: Bidirectional UNet Exchange ==============
test_unet_bidirectional() {
    local VALUE1=$((RANDOM % 1000 + 100))
    local VALUE2=$((RANDOM % 1000 + 2000))

    echo "Setting TestValue1_AS@Node1 = $VALUE1"
    uniset2-admin --confile "$CONFFILE" --setValue TestValue1_AS@Node1=$VALUE1

    echo "Setting TestValue2_AS@Node2 = $VALUE2"
    uniset2-admin --confile "$CONFFILE" --setValue TestValue2_AS@Node2=$VALUE2

    sleep 3

    local RESULT1=$(uniset2-admin --confile "$CONFFILE" --getValue TestValue1_AS@Node2 2>&1 | grep "value:" | awk '{print $2}')
    local RESULT2=$(uniset2-admin --confile "$CONFFILE" --getValue TestValue2_AS@Node1 2>&1 | grep "value:" | awk '{print $2}')

    echo "Node1->Node2: expected $VALUE1, got $RESULT1"
    echo "Node2->Node1: expected $VALUE2, got $RESULT2"

    if [ "$RESULT1" = "$VALUE1" ] && [ "$RESULT2" = "$VALUE2" ]; then
        return 0
    else
        return 1
    fi
}

# ============== Test 3: Multiple sensors ==============
test_multiple_sensors() {
    local V1=$((RANDOM % 500))
    local V2=$((RANDOM % 500 + 500))
    local B1=1
    local B2=0

    # Sensors belong to different nodes: TestValue1_AS/TestBool1_S -> Node1, TestValue2_AS/TestBool2_S -> Node2
    echo "Setting sensors on their owner nodes..."
    uniset2-admin --confile "$CONFFILE" \
        --setValue TestValue1_AS@Node1=$V1,TestBool1_S@Node1=$B1
    uniset2-admin --confile "$CONFFILE" \
        --setValue TestValue2_AS@Node2=$V2,TestBool2_S@Node2=$B2

    sleep 3

    # Read from opposite nodes to verify UNet propagation
    local R1=$(uniset2-admin --confile "$CONFFILE" --getValue TestValue1_AS@Node2 2>&1 | grep "value:" | awk '{print $2}')
    local R2=$(uniset2-admin --confile "$CONFFILE" --getValue TestValue2_AS@Node1 2>&1 | grep "value:" | awk '{print $2}')
    local RB1=$(uniset2-admin --confile "$CONFFILE" --getValue TestBool1_S@Node2 2>&1 | grep "value:" | awk '{print $2}')
    local RB2=$(uniset2-admin --confile "$CONFFILE" --getValue TestBool2_S@Node1 2>&1 | grep "value:" | awk '{print $2}')

    echo "TestValue1_AS (Node1->Node2): $V1 -> $R1"
    echo "TestValue2_AS (Node2->Node1): $V2 -> $R2"
    echo "TestBool1_S (Node1->Node2): $B1 -> $RB1"
    echo "TestBool2_S (Node2->Node1): $B2 -> $RB2"

    if [ "$R1" = "$V1" ] && [ "$R2" = "$V2" ] && [ "$RB1" = "$B1" ] && [ "$RB2" = "$B2" ]; then
        return 0
    else
        return 1
    fi
}

# Run tests
echo "=== Running Tests ==="
echo ""

run_test "SM Exchange via UNet" test_unet_exchange || true
run_test "Bidirectional UNet Exchange" test_unet_bidirectional || true
run_test "Multiple sensors sync" test_multiple_sensors || true

# Summary
echo ""
echo "=== Test Results ==="
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo ""

if [ $FAILED -gt 0 ]; then
    echo "=== TESTS FAILED ==="
    exit 1
else
    echo "=== ALL TESTS PASSED ==="
    exit 0
fi
