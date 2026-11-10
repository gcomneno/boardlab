#!/usr/bin/env bash

STATUS=0
FAILED_STEP="NONE"

HERE="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
ROOT="$(CDPATH= cd -- "$HERE/../.." && pwd)"
CF="$ROOT/cloudflare/dado-goal"
WORKER_LOG="${TMPDIR:-/tmp}/dado-goal-browser-smoke-worker.log"

NODE_RUNTIME="${DADO_GOAL_NODE_RUNTIME:-node}"
WRANGLER_CLI="${DADO_GOAL_WRANGLER_CLI:-}"

cd "$HERE"
STEP_STATUS=$?
if [ "$STEP_STATUS" -ne 0 ]; then
    STATUS=$STEP_STATUS
    FAILED_STEP="cd_harness"
fi

clear
echo "===== DADO GOAL — BROWSER RELEASE SMOKE ====="

if [ "$STATUS" -eq 0 ]; then
    npm ci
    STEP_STATUS=$?
    if [ "$STEP_STATUS" -ne 0 ]; then
        STATUS=$STEP_STATUS
        FAILED_STEP="npm_ci"
    fi
fi

if [ "$STATUS" -eq 0 ]; then
    if [ -n "$WRANGLER_CLI" ]; then
        "$NODE_RUNTIME" "$WRANGLER_CLI" \
            d1 migrations apply dado-goal-multiplayer \
            --local \
            --config "$CF/wrangler.jsonc"
    else
        npx wrangler \
            d1 migrations apply dado-goal-multiplayer \
            --local \
            --config "$CF/wrangler.jsonc"
    fi

    STEP_STATUS=$?
    if [ "$STEP_STATUS" -ne 0 ]; then
        STATUS=$STEP_STATUS
        FAILED_STEP="local_d1_migrations"
    fi
fi

if [ "$STATUS" -eq 0 ]; then
    rm -f "$WORKER_LOG"

    if [ -n "$WRANGLER_CLI" ]; then
        "$NODE_RUNTIME" "$WRANGLER_CLI" \
            dev \
            --local \
            --port 8787 \
            --config "$CF/wrangler.jsonc" \
            >"$WORKER_LOG" 2>&1 &
    else
        npx wrangler \
            dev \
            --local \
            --port 8787 \
            --config "$CF/wrangler.jsonc" \
            >"$WORKER_LOG" 2>&1 &
    fi

    WORKER_PID=$!
    STEP_STATUS=$?

    if [ "$STEP_STATUS" -ne 0 ]; then
        STATUS=$STEP_STATUS
        FAILED_STEP="start_local_worker"
    fi
fi

if [ "$STATUS" -eq 0 ]; then
    READY=0

    for _ in $(seq 1 30); do
        HTTP="$(curl -sS -o /dev/null -w '%{http_code}' \
            http://127.0.0.1:8787/multiplayer.html 2>/dev/null)"

        if [ "$HTTP" = "200" ]; then
            READY=1
            break
        fi

        sleep 1
    done

    if [ "$READY" -ne 1 ]; then
        STATUS=1
        FAILED_STEP="local_worker_readiness"
    fi
fi

if [ "$STATUS" -eq 0 ]; then
    PLAYWRIGHT_CHROMIUM_EXECUTABLE_PATH="${PLAYWRIGHT_CHROMIUM_EXECUTABLE_PATH:-/snap/bin/chromium}" \
        npx playwright test
    STEP_STATUS=$?

    if [ "$STEP_STATUS" -ne 0 ]; then
        STATUS=$STEP_STATUS
        FAILED_STEP="browser_smoke"
    fi
fi

if [ -n "${WORKER_PID:-}" ]; then
    kill "$WORKER_PID" 2>/dev/null || true
    wait "$WORKER_PID" 2>/dev/null || true
fi

if [ "$STATUS" -ne 0 ]; then
    echo
    echo "--- LOCAL WORKER LOG TAIL ---"
    tail -80 "$WORKER_LOG" 2>/dev/null || true
fi

echo
echo "FAILED_STEP=$FAILED_STEP"
echo "COMMAND_STATUS=$STATUS"
[ "$STATUS" -eq 0 ] \
    && echo "DADO_GOAL_BROWSER_SMOKE_GATE=PASS" \
    || echo "DADO_GOAL_BROWSER_SMOKE_GATE=FAIL"

[ "$STATUS" -eq 0 ]
