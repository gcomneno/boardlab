#!/usr/bin/env bash

STATUS=0
FAILED_STEP="NONE"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE="$ROOT/data/football/catalogues"

TARGETS=(
    "$ROOT/web/dado-goal/catalogues"
    "$ROOT/cloudflare/dado-goal/public/catalogues"
)

FILES=(
    championship-2024.json
    premier-league-2024.json
    serie-a-2024.json
    serie-b-2024.json
)

for TARGET in "${TARGETS[@]}"; do
    if [ "$STATUS" -eq 0 ]; then
        mkdir -p "$TARGET"
        S=$?
        if [ "$S" -ne 0 ]; then
            STATUS=$S
            FAILED_STEP="CREATE_TARGET"
        fi
    fi

    for FILE in "${FILES[@]}"; do
        if [ "$STATUS" -eq 0 ]; then
            cp -- "$SOURCE/$FILE" "$TARGET/$FILE"
            S=$?
            if [ "$S" -ne 0 ]; then
                STATUS=$S
                FAILED_STEP="COPY_$FILE"
            fi
        fi
    done
done

if [ "$STATUS" -eq 0 ]; then
    for FILE in "${FILES[@]}"; do
        cmp -s "$SOURCE/$FILE" "$ROOT/web/dado-goal/catalogues/$FILE"
        S=$?
        if [ "$S" -ne 0 ]; then
            STATUS=$S
            FAILED_STEP="VERIFY_WEB_$FILE"
        fi

        if [ "$STATUS" -eq 0 ]; then
            cmp -s "$SOURCE/$FILE" "$ROOT/cloudflare/dado-goal/public/catalogues/$FILE"
            S=$?
            if [ "$S" -ne 0 ]; then
                STATUS=$S
                FAILED_STEP="VERIFY_CLOUDFLARE_$FILE"
            fi
        fi
    done
fi

printf 'FAILED_STEP=%s\n' "$FAILED_STEP"
printf 'COMMAND_STATUS=%s\n' "$STATUS"

if [ "$STATUS" -eq 0 ]; then
    printf 'FOOTBALL_CATALOGUE_SYNC_GATE=PASS\n'
else
    printf 'FOOTBALL_CATALOGUE_SYNC_GATE=FAIL\n'
fi
