#!/usr/bin/env bash

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cd "$ROOT" || {
    printf '%s\n' "ERRORE: impossibile entrare nel repository."
    exit 1
}

FOUND=0

while IFS= read -r -d '' path; do
    lower="${path,,}"

    case "$lower" in
        sources/private/*|private/*|*/private/*|notes-private/*|*/notes-private/*)
            printf 'ERRORE: materiale privato pubblico/tracciabile: %s\n' "$path"
            FOUND=1
            continue
            ;;
    esac

    case "$lower" in
        *.pdf|*.epub|*.mobi|*.azw3)
            printf 'ERRORE: documento sorgente/binario vietato: %s\n' "$path"
            FOUND=1
            ;;
        *.mp3|*.wav|*.flac|*.m4a|*.aac|*.ogg)
            printf 'ERRORE: file audio pubblico rilevato: %s\n' "$path"
            FOUND=1
            ;;
        *.mp4|*.mkv|*.avi|*.mov|*.webm|*.mpeg|*.mpg|*.m4v)
            printf 'ERRORE: file video pubblico rilevato: %s\n' "$path"
            FOUND=1
            ;;
    esac

    case "$lower" in
        transcript/*|transcripts/*|*/transcript/*|*/transcripts/*)
            printf 'ERRORE: transcript pubblico rilevato: %s\n' "$path"
            FOUND=1
            ;;
    esac

    case "$lower" in
        sources/*.png|sources/*.jpg|sources/*.jpeg|sources/*.gif|sources/*.webp|sources/*.bmp|sources/*.tif|sources/*.tiff|\
        sources/*/*.png|sources/*/*.jpg|sources/*/*.jpeg|sources/*/*.gif|sources/*/*.webp|sources/*/*.bmp|sources/*/*.tif|sources/*/*.tiff)
            printf 'ERRORE: immagine sorgente pubblica rilevata sotto sources/: %s\n' "$path"
            FOUND=1
            ;;
    esac
done < <(
    git ls-files -z --cached --others --exclude-standard
)

if [ "$FOUND" -ne 0 ]; then
    printf '%s\n' "CONTROLLO CONTENUTO PUBBLICO FALLITO"
    exit 1
fi

printf '%s\n' "CONTROLLO CONTENUTO PUBBLICO SUPERATO"
