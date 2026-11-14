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
        npx --yes wrangler@4.160.1 \
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
    CATALOGUE_SQL="${TMPDIR:-/tmp}/dado-goal-browser-smoke-catalogues.sql"
    rm -f "$CATALOGUE_SQL"

    for CATALOGUE in \
        "$CF/public/catalogues/serie-a-2024.json" \
        "$CF/public/catalogues/serie-b-2024.json" \
        "$CF/public/catalogues/premier-league-2024.json" \
        "$CF/public/catalogues/championship-2024.json"
    do
        "$NODE_RUNTIME" "$CF/persist-football-cli.mjs" "$CATALOGUE" \
            >>"$CATALOGUE_SQL"
        STEP_STATUS=$?

        if [ "$STEP_STATUS" -ne 0 ]; then
            STATUS=$STEP_STATUS
            FAILED_STEP="prepare_local_football_catalogues"
            break
        fi
    done
fi

if [ "$STATUS" -eq 0 ]; then
    if [ -n "$WRANGLER_CLI" ]; then
        "$NODE_RUNTIME" "$WRANGLER_CLI" \
            d1 execute dado-goal-multiplayer \
            --local \
            --config "$CF/wrangler.jsonc" \
            --file "$CATALOGUE_SQL"
    else
        npx --yes wrangler@4.160.1 \
            d1 execute dado-goal-multiplayer \
            --local \
            --config "$CF/wrangler.jsonc" \
            --file "$CATALOGUE_SQL"
    fi

    STEP_STATUS=$?
    rm -f "$CATALOGUE_SQL"

    if [ "$STEP_STATUS" -ne 0 ]; then
        STATUS=$STEP_STATUS
        FAILED_STEP="seed_local_football_catalogues"
    fi
fi

if [ "$STATUS" -eq 0 ]; then
    rm -f "$WORKER_LOG"

    if [ -n "$WRANGLER_CLI" ]; then
        setsid "$NODE_RUNTIME" "$WRANGLER_CLI" \
            dev \
            --local \
            --port 8787 \
            --config "$CF/wrangler.jsonc" \
            >"$WORKER_LOG" 2>&1 &
    else
        setsid npx --yes wrangler@4.160.1 \
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
    if [ -n "${CI:-}" ]; then
        npx playwright test
    else
        PLAYWRIGHT_CHROMIUM_EXECUTABLE_PATH="${PLAYWRIGHT_CHROMIUM_EXECUTABLE_PATH:-/snap/bin/chromium}" \
            npx playwright test
    fi
    STEP_STATUS=$?

    if [ "$STEP_STATUS" -ne 0 ]; then
        STATUS=$STEP_STATUS
        FAILED_STEP="browser_smoke"
    fi
fi

if [ -n "${WORKER_PID:-}" ]; then
    kill -- "-$WORKER_PID" 2>/dev/null || true
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
