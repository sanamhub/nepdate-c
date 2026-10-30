#!/usr/bin/env bash
# Exercises scripts/published-issue.sh against a fake gh: it opens the issue when none exists,
# comments when the failing output changed, and stays silent when a later run finds the same
# output with a new run link.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
export GH_STATE="$tmp/state"
mkdir -p "$GH_STATE" "$tmp/bin" "$tmp/logs/weekly-npm"
echo '[]' > "$GH_STATE/issues.json"

cat > "$tmp/bin/gh" <<'FAKE'
#!/usr/bin/env bash
set -euo pipefail
state="${GH_STATE:?}"
command="${1:-}"
shift || true
case "$command" in
  issue)
    sub="${1:-}"
    shift || true
    case "$sub" in
      list) cat "$state/issues.json" ;;
      view)
        number="$1"
        jq --argjson n "$number" '.[] | select(.number == $n) | {body: .body, comments: .comments}' \
          "$state/issues.json"
        ;;
      create)
        title=""
        body=""
        while [ $# -gt 0 ]; do
          case "$1" in
            --title) title="$2"; shift 2 ;;
            --body-file) body="$(cat "$2")"; shift 2 ;;
            *) shift ;;
          esac
        done
        jq --arg t "$title" --arg b "$body" \
          '. + [{number: (length + 1), title: $t, body: $b, comments: []}]' \
          "$state/issues.json" > "$state/new.json"
        mv "$state/new.json" "$state/issues.json"
        echo "https://github.com/sanamhub/nepdate-c/issues/1"
        ;;
      comment)
        number="$1"
        shift
        body=""
        while [ $# -gt 0 ]; do
          case "$1" in
            --body-file) body="$(cat "$2")"; shift 2 ;;
            *) shift ;;
          esac
        done
        jq --argjson n "$number" --arg b "$body" \
          'map(if .number == $n then .comments += [{body: $b}] else . end)' \
          "$state/issues.json" > "$state/new.json"
        mv "$state/new.json" "$state/issues.json"
        echo "https://github.com/sanamhub/nepdate-c/issues/${number}#issuecomment-1"
        ;;
      *)
        echo "fake gh: unsupported issue subcommand $sub" >&2
        exit 1
        ;;
    esac
    ;;
  *)
    echo "fake gh: unsupported command $command" >&2
    exit 1
    ;;
esac
FAKE
chmod +x "$tmp/bin/gh"
export PATH="$tmp/bin:$PATH"

run() {
  GITHUB_RUN_ID="$1" GITHUB_REPOSITORY=sanamhub/nepdate-c \
    GITHUB_SERVER_URL=https://github.com GH_TOKEN=fake \
    bash "$root/scripts/published-issue.sh" nepdate 0.1.0-beta.1 "$tmp/logs"
}

comments() {
  jq '.[0].comments | length' "$GH_STATE/issues.json"
}

echo "== no open issue"
echo "error: could not compile nepdate" > "$tmp/logs/weekly-npm/out.txt"
out="$(run 101)"
grep -q 'https://github.com/sanamhub/nepdate-c/issues/1' <<<"$out"
grep -q '"title": "Published nepdate 0.1.0-beta.1 does not install or run"' "$GH_STATE/issues.json"
grep -q 'actions/runs/101' "$GH_STATE/issues.json"
grep -q 'could not compile nepdate' "$GH_STATE/issues.json"

echo "== same output, new run link"
out="$(run 102)"
grep -q 'already reports this failure' <<<"$out"
[ "$(comments)" = 0 ]
echo "ok: no comment"

echo "== output changed"
echo "error: the registry served a different tarball" > "$tmp/logs/weekly-npm/out.txt"
out="$(run 103)"
grep -q 'commented on issue #1' <<<"$out"
[ "$(comments)" = 1 ]
grep -q 'different tarball' "$GH_STATE/issues.json"
echo "ok: one comment"

echo "== same changed output again"
out="$(run 104)"
grep -q 'already reports this failure' <<<"$out"
[ "$(comments)" = 1 ]
echo "ok: still one comment"

echo "test-published-issue: all cases passed"
