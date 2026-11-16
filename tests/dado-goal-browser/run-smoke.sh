#!/usr/bin/env bash

STATUS=0
FAILED_STEP="NONE"

HERE="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
ROOT="$(CDPATH= cd -- "$HERE/../.." && pwd)"
CF="$ROOT/cloudflare/dado-goal"
WORKER_LOG="${TMPDIR:-/tmp}/dado-goal-browser-smoke-worker.log"

# Wrangler 4.160.1 requires Node >= 22.12.0.
# Honour an explicit runtime first; otherwise select a compatible local Node.
NODE_RUNTIME="${DADO_GOAL_NODE_RUNTIME:-}"

node_runtime_supported() {
    local runtime="$1"
    local version major minor

    [ -x "$runtime" ] || command -v "$runtime" >/dev/null 2>&1 || return 1

    version="$("$runtime" -p 'process.versions.node' 2>/dev/null)" || return 1
    major="${version%%.*}"
    version="${version#*.}"
    minor="${version%%.*}"

    [ "$major" -gt 22 ] || {
        [ "$major" -eq 22 ] && [ "$minor" -ge 12 ]
    }
}

if [ -n "$NODE_RUNTIME" ]; then
    if ! node_runtime_supported "$NODE_RUNTIME"; then
        printf 'Unsupported DADO_GOAL_NODE_RUNTIME: %s (Node >= 22.12.0 required)\n' \
            "$NODE_RUNTIME" >&2
        STATUS=1
        FAILED_STEP="node_runtime"
    fi
else
    CURRENT_NODE="$(command -v node 2>/dev/null || true)"

    if [ -n "$CURRENT_NODE" ] && node_runtime_supported "$CURRENT_NODE"; then
        NODE_RUNTIME="$CURRENT_NODE"
    else
        while IFS= read -r CANDIDATE; do
            if node_runtime_supported "$CANDIDATE"; then
                NODE_RUNTIME="$CANDIDATE"
                break
            fi
        done < <(
            find "$HOME/.nvm/versions/node" \
                -mindepth 3 -maxdepth 3 \
                -type f -name node -path '*/bin/node' \
                2>/dev/null |
            sort -V -r
        )
    fi

    if [ -z "$NODE_RUNTIME" ]; then
        printf 'No compatible Node runtime found (Node >= 22.12.0 required)\n' >&2
        STATUS=1
        FAILED_STEP="node_runtime"
    fi
fi

if [ "$STATUS" -eq 0 ]; then
    NODE_BIN_DIR="$(dirname "$NODE_RUNTIME")"
    PATH="$NODE_BIN_DIR:$PATH"
    export PATH
fi
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
        "$CF/public/catalogues/championship-2024.json" \
        "$CF/public/catalogues/ligue-1-2024.json" \
        "$CF/public/catalogues/bundesliga-2024.json" \
        "$CF/public/catalogues/la-liga-2024.json"
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
