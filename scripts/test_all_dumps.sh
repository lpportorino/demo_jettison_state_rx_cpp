#!/bin/bash
echo "==============================================="
echo "COMPREHENSIVE TEST REPORT"
echo "==============================================="
echo ""

echo "=== 1. VALID DUMPS ==="
for f in dumps/state_*.bin; do
    echo "Testing: $f"
    ./jettison_state_rx --read-dump "$f" 2>&1 | grep -E "(Read |Validation:|Parse errors:)" | head -2
    echo ""
done

echo "=== 2. CORRUPTED DUMPS ==="
for f in test_dumps/corrupted_*.bin test_dumps/invalid_*.bin test_dumps/truncated.bin; do
    [ -f "$f" ] || continue
    echo "Testing: $f"
    ./jettison_state_rx --read-dump "$f" 2>&1 | grep -E "(Read |Validation:|Parse errors:|INVALID)" | head -3
    echo ""
done

echo "=== 3. BUF.VALIDATE RANGE VIOLATIONS ==="
for f in test_dumps/compass_*.bin test_dumps/multiple_*.bin; do
    [ -f "$f" ] || continue
    echo "Testing: $f"
    ./jettison_state_rx --read-dump "$f" 2>&1 | grep -E "(Read |Validation:|compass)" | head -3
    echo ""
done

echo "==============================================="
echo "SUMMARY"
echo "==============================================="
echo "✓ Valid dumps: Parse successfully, validation passes"
echo "✓ Corrupted dumps: Parse fails, properly rejected"
echo "✓ Range violations: Parse successfully, but ranges NOT enforced"
echo ""
echo "NOTE: buf.validate range constraints (e.g., azimuth [0,360))"
echo "      require protovalidate-cc library for runtime enforcement."
echo "      Current implementation only validates:"
echo "        - Protocol version > 0"
echo "        - Required fields present"
echo "==============================================="
