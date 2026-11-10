#!/usr/bin/env python3

import argparse
import json
import subprocess
import sys
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONTRACT = ROOT / "docs/games/dado-goal/distribution-contract.json"


class Validation:
    def __init__(self):
        self.failed = False

    def check(self, condition, name, detail=""):
        state = "PASS" if condition else "FAIL"
        suffix = f" — {detail}" if detail else ""
        print(f"{name}={state}{suffix}")
        if not condition:
            self.failed = True


def fetch(url):
    request = urllib.request.Request(
        url,
        headers={"User-Agent": "boardlab-dado-goal-distribution-validator/1"},
    )
    with urllib.request.urlopen(request, timeout=15) as response:
        return response.status, response.read().decode("utf-8", errors="replace")


def gh_json(*args):
    completed = subprocess.run(
        ["gh", *args],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        raise RuntimeError(completed.stderr.strip() or "gh command failed")
    return json.loads(completed.stdout)


def current_repository_text():
    roots = [
        ROOT / "README.md",
        ROOT / "docs",
        ROOT / "web/dado-goal",
        ROOT / "cloudflare/dado-goal/public",
    ]
    suffixes = {".md", ".html", ".mjs", ".css", ".json"}
    chunks = []

    for root in roots:
        if root.is_file():
            chunks.append(root.read_text(errors="replace"))
            continue

        if not root.exists():
            continue

        for path in root.rglob("*"):
            if path == DEFAULT_CONTRACT:
                continue
            if path.is_file() and path.suffix in suffixes:
                chunks.append(path.read_text(errors="replace"))

    return "\n".join(chunks)


def validate_release(v, repository, tag, label):
    data = gh_json(
        "release", "view", tag,
        "--repo", repository,
        "--json", "tagName,isDraft,isPrerelease,targetCommitish,url",
    )

    v.check(data.get("tagName") == tag, f"{label}_TAG")
    v.check(not data.get("isDraft"), f"{label}_NOT_DRAFT")
    v.check(not data.get("isPrerelease"), f"{label}_NOT_PRERELEASE")

    target = data.get("targetCommitish", "")
    commit = subprocess.run(
        ["git", "cat-file", "-e", f"{target}^{{commit}}"],
        cwd=ROOT,
        check=False,
        capture_output=True,
    )
    v.check(commit.returncode == 0, f"{label}_TARGET_COMMIT", target)


def main():
    parser = argparse.ArgumentParser(
        description="Read-only Dado Goal public distribution validator."
    )
    parser.add_argument(
        "--offline",
        action="store_true",
        help="validate only repository-local distribution contracts",
    )
    parser.add_argument(
        "--contract",
        type=Path,
        default=DEFAULT_CONTRACT,
    )
    args = parser.parse_args()

    contract = json.loads(args.contract.read_text())
    v = Validation()

    print("===== DADO GOAL — PUBLIC DISTRIBUTION VALIDATION =====")

    repository_url = contract["repository"]
    repository_slug = repository_url.removeprefix("https://github.com/")
    itch = contract["itch_page"]
    stable = contract["stable_multiplayer"]
    single_tag = contract["releases"]["single"]
    multiplayer_tag = contract["releases"]["multiplayer"]

    local_text = current_repository_text()

    v.check(repository_url in local_text, "REPOSITORY_CANONICAL_URL")
    v.check(stable in local_text, "REPOSITORY_STABLE_MULTIPLAYER")
    v.check(
        all(item not in local_text
            for item in contract["obsolete_current_references"]),
        "REPOSITORY_OBSOLETE_REFERENCES",
    )

    if args.offline:
        print("NETWORK_CHECKS=SKIPPED")
        print(f"COMMAND_STATUS={1 if v.failed else 0}")
        print(
            "DADO_GOAL_DISTRIBUTION_GATE="
            + ("FAIL" if v.failed else "PASS")
        )
        return 1 if v.failed else 0

    try:
        validate_release(v, repository_slug, single_tag, "SINGLE_RELEASE")
        validate_release(
            v, repository_slug, multiplayer_tag, "MULTIPLAYER_RELEASE"
        )
    except (RuntimeError, json.JSONDecodeError) as error:
        v.check(False, "GITHUB_RELEASES", str(error))

    try:
        status, itch_html = fetch(itch)
        v.check(status == 200, "ITCH_PAGE_HTTP", f"HTTP {status}")
        v.check(
            "html-classic.itch.zone/html/" in itch_html,
            "ITCH_BROWSER_BUILD",
        )
        v.check(stable in itch_html, "ITCH_STABLE_MULTIPLAYER")
        v.check(
            all(item not in itch_html
                for item in contract["obsolete_current_references"]),
            "ITCH_OBSOLETE_REFERENCES",
        )
    except (urllib.error.URLError, TimeoutError) as error:
        v.check(False, "ITCH_NETWORK", str(error))

    for language, url in contract["showcase"].items():
        label = language.upper()
        try:
            status, text = fetch(url)
            v.check(status == 200, f"SHOWCASE_{label}_HTTP", f"HTTP {status}")
            v.check(itch in text, f"SHOWCASE_{label}_ITCH")
            v.check(single_tag in text, f"SHOWCASE_{label}_SINGLE_RELEASE")
            v.check(
                multiplayer_tag in text,
                f"SHOWCASE_{label}_MULTIPLAYER_RELEASE",
            )
            v.check(stable in text, f"SHOWCASE_{label}_STABLE_MULTIPLAYER")
            v.check(
                all(item not in text
                    for item in contract["obsolete_current_references"]),
                f"SHOWCASE_{label}_OBSOLETE_REFERENCES",
            )
        except (urllib.error.URLError, TimeoutError) as error:
            v.check(False, f"SHOWCASE_{label}_NETWORK", str(error))

    origin = contract["stable_worker_origin"]
    for asset in contract["worker_assets"]:
        try:
            status, _ = fetch(origin + asset)
            name = asset.rsplit("/", 1)[-1].replace(".", "_").upper()
            v.check(status == 200, f"WORKER_{name}_HTTP", f"HTTP {status}")
        except (urllib.error.URLError, TimeoutError) as error:
            v.check(False, f"WORKER_NETWORK_{asset}", str(error))

    print("VALIDATOR_MODE=READ_ONLY")
    print(f"COMMAND_STATUS={1 if v.failed else 0}")
    print(
        "DADO_GOAL_DISTRIBUTION_GATE="
        + ("FAIL" if v.failed else "PASS")
    )
    return 1 if v.failed else 0


if __name__ == "__main__":
    sys.exit(main())
