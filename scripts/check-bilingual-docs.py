#!/usr/bin/env python3
"""Validate registered English/Italian Markdown document pairs."""

from __future__ import annotations

import html
import json
import re
import subprocess
import sys
import unicodedata
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parent.parent
MANIFEST_PATH = ROOT / ".github" / "bilingual-docs.json"

HEADING_RE = re.compile(r"^(#{1,6})[ \t]+(.+?)\s*$")
FENCE_RE = re.compile(r"^[ \t]{0,3}(`{3,}|~{3,})(.*)$")
INLINE_CODE_RE = re.compile(r"(?<!`)`([^`\n]+)`(?!`)")
LINK_RE = re.compile(r"!?\[[^\]]*]\(([^)]+)\)")
URI_SCHEME_RE = re.compile(r"^[A-Za-z][A-Za-z0-9+.-]*:")


@dataclass(frozen=True)
class MarkdownDocument:
    path: Path
    text: str
    prose: str
    heading_levels: tuple[int, ...]
    code_blocks: tuple[tuple[str, str], ...]
    inline_code: Counter[str]
    links: tuple[str, ...]


def relative_name(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def parse_link_target(raw_target: str) -> str:
    target = raw_target.strip()

    if target.startswith("<") and ">" in target:
        return target[1 : target.index(">")].strip()

    return target.split(maxsplit=1)[0] if target else ""


def markdown_heading_text(raw_heading: str) -> str:
    heading = re.sub(
        r"!\[([^\]]*)\]\([^)]+\)",
        r"\1",
        raw_heading,
    )
    heading = re.sub(
        r"\[([^\]]+)\]\([^)]+\)",
        r"\1",
        heading,
    )
    heading = re.sub(r"<[^>]+>", "", heading)
    heading = heading.replace("`", "")
    heading = heading.replace("*", "")
    heading = heading.replace("~", "")
    return html.unescape(heading).strip()


def github_slug_base(raw_heading: str) -> str:
    heading = markdown_heading_text(raw_heading)
    heading = re.sub(r"\s+", "-", heading.lower().strip())

    characters: list[str] = []

    for character in heading:
        category = unicodedata.category(character)

        if character in {"-", "_"} or character.isalnum() or category.startswith("M"):
            characters.append(character)

    return "".join(characters)


def github_heading_anchors(text: str) -> tuple[str, ...]:
    anchors: list[str] = []
    occurrences: Counter[str] = Counter()

    in_fence = False
    fence_character = ""
    fence_length = 0

    for line in text.splitlines():
        if not in_fence:
            fence_match = FENCE_RE.match(line)

            if fence_match:
                marker = fence_match.group(1)
                in_fence = True
                fence_character = marker[0]
                fence_length = len(marker)
                continue

            heading_match = HEADING_RE.match(line)

            if not heading_match:
                continue

            raw_heading = re.sub(
                r"\s+#+\s*$",
                "",
                heading_match.group(2),
            ).strip()

            base = github_slug_base(raw_heading)
            occurrence = occurrences[base]
            occurrences[base] += 1

            anchor = base if occurrence == 0 else f"{base}-{occurrence}"
            anchors.append(anchor)
            continue

        stripped = line.lstrip()
        closing = stripped.strip()

        if (
            closing.startswith(fence_character * fence_length)
            and set(closing) <= {fence_character}
            and len(closing) >= fence_length
        ):
            in_fence = False
            fence_character = ""
            fence_length = 0

    return tuple(anchors)


def strip_indentation(
    line: str,
    required_width: int,
) -> str | None:
    position = 0
    width = 0

    while position < len(line) and width < required_width:
        character = line[position]

        if character == " ":
            width += 1
            position += 1
            continue

        if character == "\t":
            next_width = width + 4 - (width % 4)
            position += 1

            if next_width > required_width:
                return " " * (next_width - required_width) + line[position:]

            width = next_width
            continue

        return None

    if width < required_width:
        return None

    return line[position:]


def indented_code_content(line: str) -> str | None:
    return strip_indentation(line, 4)


def indentation_width(line: str) -> int:
    width = 0

    for character in line:
        if character == " ":
            width += 1
            continue

        if character == "\t":
            width += 4 - (width % 4)
            continue

        break

    return width


def is_thematic_break(line: str) -> bool:
    compact = line.strip().replace(" ", "").replace("\t", "")

    return len(compact) >= 3 and compact[0] in "*-_" and set(compact) == {compact[0]}


def list_item_content_indent(line: str) -> int | None:
    if is_thematic_break(line):
        return None

    match = re.match(
        r"^( {0,3})([-+*]|\d{1,9}[.)])([ \t]+|$)",
        line,
    )

    if match is None:
        return None

    marker_indent = len(match.group(1))
    marker_width = len(match.group(2))
    whitespace = match.group(3)
    whitespace_width = indentation_width(whitespace)

    padding = whitespace_width if 1 <= whitespace_width <= 4 else 1

    return marker_indent + marker_width + padding


def active_list_content_indent(
    lines: list[str],
    index: int,
) -> int | None:
    cursor = index - 1
    immediate_blank_count = 0

    while cursor >= 0 and not lines[cursor].strip():
        immediate_blank_count += 1
        cursor -= 1

    if immediate_blank_count == 0 or immediate_blank_count >= 2:
        return None

    intervening_lines: list[str] = []
    blank_run = 0

    while cursor >= 0:
        line = lines[cursor]

        if not line.strip():
            blank_run += 1

            if blank_run >= 2:
                return None

            cursor -= 1
            continue

        blank_run = 0
        content_indent = list_item_content_indent(line)

        if content_indent is not None:
            compatible = all(
                (not candidate.strip() or indentation_width(candidate) >= content_indent)
                for candidate in intervening_lines
            )

            return content_indent if compatible else None

        intervening_lines.append(line)
        cursor -= 1

    return None


def indented_code_start_indent(
    lines: list[str],
    index: int,
) -> int | None:
    current_indent = indentation_width(lines[index])

    if current_indent < 4:
        return None

    if index == 0:
        return 4

    previous_line = lines[index - 1]

    if HEADING_RE.match(previous_line):
        return 4

    if previous_line.strip():
        return None

    list_content_indent = active_list_content_indent(
        lines,
        index,
    )

    if list_content_indent is None:
        return 4

    nested_code_indent = list_content_indent + 4

    if current_indent >= nested_code_indent:
        return nested_code_indent

    if current_indent < list_content_indent:
        return 4

    return None


def parse_markdown(
    path: Path,
    errors: list[str],
) -> MarkdownDocument:
    text = path.read_text(encoding="utf-8")
    lines = text.splitlines()

    heading_levels: list[int] = []
    code_blocks: list[tuple[str, str]] = []
    prose_lines: list[str] = []

    in_fence = False
    fence_character = ""
    fence_length = 0
    fence_info = ""
    fence_body: list[str] = []

    index = 0

    while index < len(lines):
        line = lines[index]

        if in_fence:
            stripped = line.lstrip()

            if (
                stripped.startswith(fence_character * fence_length)
                and set(stripped.strip()) <= {fence_character}
                and len(stripped.strip()) >= fence_length
            ):
                code_blocks.append((fence_info, "\n".join(fence_body)))
                in_fence = False
                fence_character = ""
                fence_length = 0
                fence_info = ""
                fence_body = []
            else:
                fence_body.append(line)

            index += 1
            continue

        fence_match = FENCE_RE.match(line)

        if fence_match:
            marker = fence_match.group(1)
            in_fence = True
            fence_character = marker[0]
            fence_length = len(marker)
            fence_info = fence_match.group(2).strip()
            fence_body = []
            index += 1
            continue

        indented_code_indent = indented_code_start_indent(
            lines,
            index,
        )

        if indented_code_indent is not None:
            indented_content = strip_indentation(
                line,
                indented_code_indent,
            )

            if indented_content is None:
                raise RuntimeError("Inconsistent indented-code start")

            indented_body = [indented_content]
            index += 1

            while index < len(lines):
                candidate = lines[index]
                candidate_content = strip_indentation(
                    candidate,
                    indented_code_indent,
                )

                if candidate_content is not None:
                    indented_body.append(candidate_content)
                    index += 1
                    continue

                if not candidate.strip():
                    lookahead = index
                    blank_lines: list[str] = []

                    while lookahead < len(lines) and not lines[lookahead].strip():
                        blank_lines.append("")
                        lookahead += 1

                    if (
                        lookahead < len(lines)
                        and strip_indentation(
                            lines[lookahead],
                            indented_code_indent,
                        )
                        is not None
                    ):
                        indented_body.extend(blank_lines)
                        index = lookahead
                        continue

                break

            code_blocks.append(("", "\n".join(indented_body)))
            continue

        heading_match = HEADING_RE.match(line)

        if heading_match:
            heading_levels.append(len(heading_match.group(1)))

        prose_lines.append(line)
        index += 1

    if in_fence:
        errors.append(f"{relative_name(path)}: blocco di codice non chiuso")

    prose = "\n".join(prose_lines)
    inline_code = Counter(INLINE_CODE_RE.findall(prose))
    links = tuple(
        parse_link_target(raw) for raw in LINK_RE.findall(prose) if parse_link_target(raw)
    )

    return MarkdownDocument(
        path=path,
        text=text,
        prose=prose,
        heading_levels=tuple(heading_levels),
        code_blocks=tuple(code_blocks),
        inline_code=inline_code,
        links=links,
    )


def translation_for(canonical: str, suffix: str) -> str:
    if not canonical.endswith(".md"):
        raise ValueError(f"Il documento canonico non termina con .md: {canonical}")

    if canonical.endswith(suffix):
        raise ValueError(f"Il documento canonico usa già il suffisso italiano: {canonical}")

    return canonical[:-3] + suffix


def split_local_target(target: str) -> tuple[str, str]:
    path_and_query, separator, fragment = target.partition("#")
    path_part = path_and_query.split("?", 1)[0]

    return (
        unquote(path_part),
        unquote(fragment) if separator else "",
    )


def resolve_local_target(
    document: MarkdownDocument,
    target: str,
) -> tuple[Path, str] | None:
    path_part, fragment = split_local_target(target)

    candidate = document.path if not path_part else document.path.parent / path_part
    resolved = candidate.resolve()

    try:
        resolved.relative_to(ROOT.resolve())
    except ValueError:
        return None

    return resolved, fragment


def validate_local_links(
    document: MarkdownDocument,
    errors: list[str],
) -> None:
    for target in document.links:
        if not target or URI_SCHEME_RE.match(target):
            continue

        resolved_target = resolve_local_target(
            document,
            target,
        )

        if resolved_target is None:
            errors.append(
                f"{relative_name(document.path)}: link locale esterno al repository: {target}"
            )
            continue

        resolved, fragment = resolved_target

        if not resolved.exists():
            errors.append(f"{relative_name(document.path)}: link locale inesistente: {target}")
            continue

        if fragment and resolved.is_file() and resolved.suffix.lower() == ".md":
            anchors = github_heading_anchors(resolved.read_text(encoding="utf-8"))

            if fragment not in anchors:
                errors.append(
                    f"{relative_name(document.path)}: anchor locale inesistente: {target}"
                )


def normalized_navigation_target(
    document: MarkdownDocument,
    target: str,
    suffix: str,
) -> str:
    if URI_SCHEME_RE.match(target):
        return target

    resolved_target = resolve_local_target(
        document,
        target,
    )

    if resolved_target is None:
        return f"outside:{target}"

    resolved, fragment = resolved_target

    try:
        relative = resolved.relative_to(ROOT.resolve()).as_posix()
    except ValueError:
        return f"outside:{target}"

    if relative.endswith(suffix):
        relative = relative[: -len(suffix)] + ".md"

    if not fragment:
        return relative

    if resolved.is_file() and resolved.suffix.lower() == ".md":
        anchors = github_heading_anchors(resolved.read_text(encoding="utf-8"))

        try:
            heading_index = anchors.index(fragment)
        except ValueError:
            return f"{relative}#missing:{fragment}"

        return f"{relative}#heading-index:{heading_index}"

    return f"{relative}#{fragment}"


def validate_language_links(
    canonical: MarkdownDocument,
    translation: MarkdownDocument,
    errors: list[str],
) -> None:
    expected = {
        canonical.path.name,
        translation.path.name,
    }

    for document in (canonical, translation):
        top_lines = "\n".join(document.text.splitlines()[:12])
        top_targets = {parse_link_target(raw) for raw in LINK_RE.findall(top_lines)}

        missing = expected - top_targets

        if missing:
            errors.append(
                f"{relative_name(document.path)}: "
                "link di lingua mancanti nelle prime 12 righe: " + ", ".join(sorted(missing))
            )


def validate_pair(
    canonical_path: Path,
    translation_path: Path,
    suffix: str,
    errors: list[str],
) -> None:
    canonical = parse_markdown(canonical_path, errors)
    translation = parse_markdown(translation_path, errors)

    validate_language_links(canonical, translation, errors)
    validate_local_links(canonical, errors)
    validate_local_links(translation, errors)

    if canonical.heading_levels != translation.heading_levels:
        errors.append(
            f"{relative_name(canonical_path)} e "
            f"{relative_name(translation_path)}: "
            "gerarchia degli heading non sincronizzata"
        )

    if canonical.code_blocks != translation.code_blocks:
        errors.append(
            f"{relative_name(canonical_path)} e "
            f"{relative_name(translation_path)}: "
            "blocchi di codice differenti"
        )

    if canonical.inline_code != translation.inline_code:
        missing_in_translation = canonical.inline_code - translation.inline_code
        extra_in_translation = translation.inline_code - canonical.inline_code

        details: list[str] = []

        if missing_in_translation:
            details.append(
                "mancanti in italiano: " + ", ".join(sorted(missing_in_translation.elements()))
            )

        if extra_in_translation:
            details.append(
                "aggiuntivi in italiano: " + ", ".join(sorted(extra_in_translation.elements()))
            )

        errors.append(
            f"{relative_name(canonical_path)} e "
            f"{relative_name(translation_path)}: "
            "riferimenti tecnici inline non sincronizzati"
            + (f" ({'; '.join(details)})" if details else "")
        )

    canonical_links = Counter(
        normalized_navigation_target(
            canonical,
            target,
            suffix,
        )
        for target in canonical.links
    )
    translation_links = Counter(
        normalized_navigation_target(
            translation,
            target,
            suffix,
        )
        for target in translation.links
    )

    if canonical_links != translation_links:
        errors.append(
            f"{relative_name(canonical_path)} e "
            f"{relative_name(translation_path)}: "
            "navigazione Markdown non equivalente"
        )


def repository_markdown_files(
    ignored_directories: set[str],
    errors: list[str],
) -> set[str]:
    result = subprocess.run(
        [
            "git",
            "ls-files",
            "-z",
            "--cached",
            "--others",
            "--exclude-standard",
            "--",
            "*.md",
        ],
        cwd=ROOT,
        capture_output=True,
        check=False,
    )

    if result.returncode != 0:
        diagnostic = result.stderr.decode(
            "utf-8",
            errors="replace",
        ).strip()

        errors.append(
            "impossibile inventariare i Markdown tramite Git"
            + (f": {diagnostic}" if diagnostic else "")
        )
        return set()

    documents: set[str] = set()

    for raw_path in result.stdout.split(b"\0"):
        if not raw_path:
            continue

        relative_name = raw_path.decode(
            "utf-8",
            errors="surrogateescape",
        )
        relative = Path(relative_name)

        if any(part in ignored_directories for part in relative.parts):
            continue

        documents.add(relative.as_posix())

    return documents


def main() -> int:
    errors: list[str] = []

    try:
        manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    except FileNotFoundError:
        print(f"ERRORE: manifest assente: {relative_name(MANIFEST_PATH)}")
        return 1
    except json.JSONDecodeError as exc:
        print(f"ERRORE: manifest JSON non valido: {exc}")
        return 1

    if not isinstance(manifest, dict):
        print("ERRORE: il manifest JSON deve essere un oggetto")
        return 1

    if manifest.get("schema_version") != 1:
        errors.append("schema_version del manifest non supportata")

    if manifest.get("canonical_language") != "en":
        errors.append("canonical_language deve essere esattamente en")

    if manifest.get("translation_language") != "it":
        errors.append("translation_language deve essere esattamente it")

    suffix = manifest.get("translation_suffix")

    if suffix != ".it.md":
        errors.append("translation_suffix deve essere esattamente .it.md")
        suffix = ".it.md"

    def read_string_list(key: str) -> list[str]:
        value = manifest.get(key)

        if not isinstance(value, list):
            errors.append(f"{key} deve essere una lista")
            return []

        if not all(isinstance(item, str) for item in value):
            errors.append(f"{key} deve contenere soltanto stringhe")
            return [item for item in value if isinstance(item, str)]

        if value != sorted(set(value)):
            errors.append(f"{key} deve essere ordinato e senza duplicati")

        return value

    canonical_documents = read_string_list("canonical_documents")
    legacy_documents = read_string_list("legacy_unpaired_documents")
    excluded_documents = read_string_list("excluded_documents")
    ignored_directories = set(read_string_list("ignored_directories"))

    categories = {
        "canonical_documents": set(canonical_documents),
        "legacy_unpaired_documents": set(legacy_documents),
        "excluded_documents": set(excluded_documents),
    }
    category_names = list(categories)

    for index, left_name in enumerate(category_names):
        for right_name in category_names[index + 1 :]:
            overlap = categories[left_name] & categories[right_name]

            if overlap:
                errors.append(
                    f"{left_name} e {right_name} si sovrappongono: " + ", ".join(sorted(overlap))
                )

    for category_name, documents in (
        (
            "legacy_unpaired_documents",
            legacy_documents,
        ),
        (
            "excluded_documents",
            excluded_documents,
        ),
    ):
        for document_name in documents:
            if not document_name.endswith(".md"):
                errors.append(f"{category_name} contiene un file non Markdown: {document_name}")
                continue

            if category_name == "legacy_unpaired_documents" and document_name.endswith(suffix):
                errors.append(
                    f"{category_name} non può contenere una traduzione italiana: {document_name}"
                )

            if not (ROOT / document_name).is_file():
                errors.append(f"{category_name} contiene un file assente: {document_name}")

    expected_translations: set[str] = set()

    for canonical_name in canonical_documents:
        if not isinstance(canonical_name, str):
            errors.append("canonical_documents contiene un valore non testuale")
            continue

        try:
            translation_name = translation_for(canonical_name, suffix)
        except ValueError as exc:
            errors.append(str(exc))
            continue

        expected_translations.add(translation_name)

        canonical_path = ROOT / canonical_name
        translation_path = ROOT / translation_name

        if not canonical_path.is_file():
            errors.append(f"documento canonico assente: {canonical_name}")

        if not translation_path.is_file():
            errors.append(f"traduzione italiana assente: {translation_name}")

        if canonical_path.is_file() and translation_path.is_file():
            validate_pair(
                canonical_path,
                translation_path,
                suffix,
                errors,
            )

    translation_overlap = expected_translations & (
        set(canonical_documents) | set(legacy_documents) | set(excluded_documents)
    )

    if translation_overlap:
        errors.append(
            "le traduzioni attese si sovrappongono ad altre "
            "categorie del manifest: " + ", ".join(sorted(translation_overlap))
        )

    all_markdown = repository_markdown_files(
        ignored_directories,
        errors,
    )

    translations_on_disk = {name for name in all_markdown if name.endswith(suffix)}

    unregistered_translations = translations_on_disk - expected_translations

    if unregistered_translations:
        errors.append(
            "traduzioni italiane non registrate "
            "nel manifest: " + ", ".join(sorted(unregistered_translations))
        )

    classified_documents = (
        set(canonical_documents)
        | expected_translations
        | set(legacy_documents)
        | set(excluded_documents)
    )

    unclassified_documents = all_markdown - classified_documents - unregistered_translations

    if unclassified_documents:
        errors.append(
            "documenti Markdown pubblici non classificati "
            "nel manifest: " + ", ".join(sorted(unclassified_documents))
        )

    if errors:
        print("VALIDAZIONE BILINGUE FALLITA")

        for error in errors:
            print(f"- {error}")

        return 1

    print("VALIDAZIONE BILINGUE SUPERATA")
    print(f"Coppie verificate: {len(canonical_documents)}")
    print(f"Documenti legacy censiti: {len(legacy_documents)}")
    print(f"Documenti esclusi censiti: {len(excluded_documents)}")

    for canonical_name in canonical_documents:
        print(f"- {canonical_name} <-> {translation_for(canonical_name, suffix)}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
