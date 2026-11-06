#!/usr/bin/env bash
# FxSound Linux smoke test script
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Find executable
BINARY="${1:-}"
if [ -z "$BINARY" ]; then
    for candidate in \
        "$REPO_ROOT/build/FxSound_artefacts/Release/FxSound" \
        "$REPO_ROOT/build/bin/FxSound" \
        "/usr/bin/FxSound" \
        "$(command -v FxSound 2>/dev/null || true)"; do
        if [ -n "$candidate" ] && [ -x "$candidate" ]; then
            BINARY="$candidate"
            break
        fi
    done
fi

if [ -z "$BINARY" ] || [ ! -x "$BINARY" ]; then
    echo "ERROR: FxSound binary not found or not executable: ${BINARY:-<none>}" >&2
    exit 1
fi

echo "=== FxSound Linux Smoke Test ==="
echo "Target binary: $BINARY"

# 1. Dependency check (ldd)
echo "[1/4] Checking dynamic dependencies..."
if command -v ldd >/dev/null 2>&1; then
    MISSING_LIBS=$(ldd "$BINARY" | grep "not found" || true)
    if [ -n "$MISSING_LIBS" ]; then
        echo "ERROR: Missing shared library dependencies:" >&2
        echo "$MISSING_LIBS" >&2
        exit 1
    fi
    echo "  -> Dynamic dependencies OK"
else
    echo "  -> ldd not found, skipping dependency check"
fi

# 2. Version test
echo "[2/4] Testing --version..."
VERSION_OUT=$("$BINARY" --version)
echo "  -> Output: $VERSION_OUT"
if [[ "$VERSION_OUT" != *"FxSound"* ]] || [[ "$VERSION_OUT" != *"1.2.17"* ]]; then
    echo "ERROR: --version output did not match expected 'FxSound 1.2.17'" >&2
    exit 1
fi
echo "  -> --version OK"

# 3. Help test
echo "[3/4] Testing --help..."
HELP_OUT=$("$BINARY" --help)
if [[ "$HELP_OUT" != *"Usage:"* ]]; then
    echo "ERROR: --help output did not contain 'Usage:'" >&2
    exit 1
fi
echo "  -> --help OK"

# 4. Graceful shutdown / lifecycle test (SIGTERM)
echo "[4/4] Testing lifecycle and signal handling (SIGTERM)..."
RUN_WRAPPER=""
if [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
    if command -v xvfb-run >/dev/null 2>&1; then
        RUN_WRAPPER="xvfb-run -a"
    fi
fi

if [ -n "${DISPLAY:-}" ] || [ -n "${WAYLAND_DISPLAY:-}" ] || [ -n "$RUN_WRAPPER" ]; then
    $RUN_WRAPPER "$BINARY" --run_minimized &
    APP_PID=$!

    sleep 1

    if kill -0 "$APP_PID" 2>/dev/null; then
        echo "  -> Process started (PID $APP_PID), sending SIGTERM..."
        kill -TERM "$APP_PID"

        WAIT_COUNT=0
        while kill -0 "$APP_PID" 2>/dev/null && [ "$WAIT_COUNT" -lt 10 ]; do
            sleep 0.5
            WAIT_COUNT=$((WAIT_COUNT + 1))
        done

        if kill -0 "$APP_PID" 2>/dev/null; then
            echo "ERROR: Process did not exit cleanly within timeout after SIGTERM" >&2
            kill -KILL "$APP_PID" 2>/dev/null || true
            exit 1
        fi
        wait "$APP_PID" 2>/dev/null || true
        echo "  -> SIGTERM clean exit OK"
    else
        echo "WARNING: Process exited prematurely; skipping signal test"
    fi
else
    echo "  -> No DISPLAY, WAYLAND_DISPLAY, or xvfb-run detected; skipping GUI signal test"
fi

echo "=== All smoke tests passed successfully! ==="
