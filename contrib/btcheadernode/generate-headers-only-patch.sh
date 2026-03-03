#!/usr/bin/env bash
# Generate a headers-only patch against the pinned Bitcoin lock commit.
set -euo pipefail

usage() {
    cat <<'EOF'
Usage:
  generate-headers-only-patch.sh --fork-repo <repo-url-or-path> [options]

Required:
  --fork-repo <repo>        Bitcoin fork repository URL/path containing headers-only changes.
                            Optional if BITCOIN_PATCH_FORK_REPO is set in bitcoin.lock.

Optional:
  --fork-ref <ref>          Fork ref to diff (default: HEAD).
  --src-root <path>         Syscoin source root (default: current directory).
  --base-commit <sha>       Override base commit (default: BITCOIN_COMMIT from bitcoin.lock).
  --output <path>           Patch output path. Relative paths are rooted at --src-root
                            (default: contrib/btcheadernode/patches/headers-only.diff).
  --keep-tmp                Keep temporary clone directory for debugging.
  -h, --help                Show this help.

Example:
  contrib/btcheadernode/generate-headers-only-patch.sh \
    --src-root "$(pwd)" \
    --fork-repo https://github.com/example/bitcoin.git \
    --fork-ref headers-only-v30.2
EOF
}

require_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Required tool not found: $1" >&2
        exit 1
    fi
}

FORK_REPO=""
FORK_REF=""
SRC_ROOT="$(pwd)"
BASE_COMMIT=""
OUTPUT_PATH="contrib/btcheadernode/patches/headers-only.diff"
KEEP_TMP=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --fork-repo)
            [[ $# -ge 2 ]] || { echo "Missing value for --fork-repo" >&2; exit 1; }
            FORK_REPO="$2"
            shift 2
            ;;
        --fork-ref)
            [[ $# -ge 2 ]] || { echo "Missing value for --fork-ref" >&2; exit 1; }
            FORK_REF="$2"
            shift 2
            ;;
        --src-root)
            [[ $# -ge 2 ]] || { echo "Missing value for --src-root" >&2; exit 1; }
            SRC_ROOT="$2"
            shift 2
            ;;
        --base-commit)
            [[ $# -ge 2 ]] || { echo "Missing value for --base-commit" >&2; exit 1; }
            BASE_COMMIT="$2"
            shift 2
            ;;
        --output)
            [[ $# -ge 2 ]] || { echo "Missing value for --output" >&2; exit 1; }
            OUTPUT_PATH="$2"
            shift 2
            ;;
        --keep-tmp)
            KEEP_TMP=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

require_cmd git

LOCK_FILE="$SRC_ROOT/contrib/btcheadernode/bitcoin.lock"
if [[ ! -f "$LOCK_FILE" ]]; then
    echo "Missing lock file: $LOCK_FILE" >&2
    exit 1
fi

# shellcheck source=/dev/null
source "$LOCK_FILE"

: "${BITCOIN_REPO:?BITCOIN_REPO must be set in bitcoin.lock}"
: "${BITCOIN_COMMIT:?BITCOIN_COMMIT must be set in bitcoin.lock}"

if [[ -z "$FORK_REPO" ]]; then
    FORK_REPO="${BITCOIN_PATCH_FORK_REPO:-}"
fi
if [[ -z "$FORK_REF" ]]; then
    FORK_REF="${BITCOIN_PATCH_FORK_REF:-HEAD}"
fi
if [[ -z "$FORK_REPO" ]]; then
    echo "Missing fork repository. Set --fork-repo or BITCOIN_PATCH_FORK_REPO in bitcoin.lock." >&2
    usage >&2
    exit 1
fi

if [[ -z "$BASE_COMMIT" ]]; then
    BASE_COMMIT="$BITCOIN_COMMIT"
fi

if [[ "$OUTPUT_PATH" = /* ]]; then
    OUTPUT_ABS="$OUTPUT_PATH"
else
    OUTPUT_ABS="$SRC_ROOT/$OUTPUT_PATH"
fi

mkdir -p "$(dirname "$OUTPUT_ABS")"

TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/btcheadernode-patch.XXXXXXXX")"
cleanup() {
    if [[ "$KEEP_TMP" -eq 0 ]]; then
        rm -rf "$TMP_DIR"
    else
        echo "Kept temp dir: $TMP_DIR"
    fi
}
trap cleanup EXIT

FORK_DIR="$TMP_DIR/fork"
UPSTREAM_DIR="$TMP_DIR/upstream"

echo "Cloning fork: $FORK_REPO"
git clone --filter=blob:none "$FORK_REPO" "$FORK_DIR"
git -C "$FORK_DIR" fetch --tags --prune origin
git -C "$FORK_DIR" checkout --detach "$FORK_REF"
FORK_COMMIT="$(git -C "$FORK_DIR" rev-parse --verify HEAD)"

if ! git -C "$FORK_DIR" cat-file -e "${BASE_COMMIT}^{commit}" 2>/dev/null; then
    git -C "$FORK_DIR" fetch origin "$BASE_COMMIT" --depth=1 || true
fi
if ! git -C "$FORK_DIR" cat-file -e "${BASE_COMMIT}^{commit}" 2>/dev/null; then
    echo "Base commit not found in fork repository: $BASE_COMMIT" >&2
    exit 1
fi

if ! git -C "$FORK_DIR" merge-base --is-ancestor "$BASE_COMMIT" "$FORK_COMMIT"; then
    echo "Base commit $BASE_COMMIT is not an ancestor of fork ref $FORK_REF ($FORK_COMMIT)." >&2
    echo "Rebase/cherry-pick your headers-only branch onto the locked base commit first." >&2
    exit 1
fi

git -C "$FORK_DIR" diff --binary --full-index --no-color "$BASE_COMMIT..$FORK_COMMIT" > "$OUTPUT_ABS"

if [[ ! -s "$OUTPUT_ABS" ]]; then
    echo "Generated patch is empty. No changes between $BASE_COMMIT and $FORK_COMMIT." >&2
    exit 1
fi

echo "Generated patch: $OUTPUT_ABS"
echo "Patch diffstat:"
git -C "$FORK_DIR" diff --stat "$BASE_COMMIT..$FORK_COMMIT"

echo "Verifying patch applies to locked upstream source..."
git clone --filter=blob:none "$BITCOIN_REPO" "$UPSTREAM_DIR"
git -C "$UPSTREAM_DIR" fetch --tags --prune origin
git -C "$UPSTREAM_DIR" checkout --detach "$BASE_COMMIT"
git -C "$UPSTREAM_DIR" apply --check "$OUTPUT_ABS"

echo "Patch verified against base commit: $BASE_COMMIT"
echo "To enable patch in builds, set BITCOIN_PATCH in bitcoin.lock to:"
echo "  ${OUTPUT_ABS#$SRC_ROOT/contrib/btcheadernode/}"
