#!/usr/bin/env bash
# Opens or updates the single issue a failed scheduled check reports through. A weekly failure is
# often the same failure, so the issue is opened once and commented on only when the failing
# output changes; a run that finds the same output says so and stops.
#
#   scripts/published-issue.sh <package> <version> <failure-directory>
#
# The directory holds one file per failed job, named after that job. The body carries the run
# link and the last lines of each file, and the comparison uses a fingerprint of the same text
# with the run link and the variable parts removed, so a run link that changes every week cannot
# turn one failure into fifty comments.
#
# Environment: GITHUB_RUN_ID, GITHUB_REPOSITORY and GITHUB_SERVER_URL, set by Actions; GH_TOKEN,
# which the report job gets from github.token. The issue is labelled published-check, which the
# maintainer creates once.
set -euo pipefail

label="published-check"
package="${1:?usage: published-issue.sh <package> <version> <failure-directory>}"
version="${2:?usage: published-issue.sh <package> <version> <failure-directory>}"
logs="${3:?usage: published-issue.sh <package> <version> <failure-directory>}"

server="${GITHUB_SERVER_URL:-https://github.com}"
run="${server}/${GITHUB_REPOSITORY}/actions/runs/${GITHUB_RUN_ID}"
title="Published ${package} ${version} does not install or run"
body="$(mktemp)"
trap 'rm -f "$body"' EXIT

# The tail of each file is what a reader needs; the rest is in the run.
tail_of() {
  tail -n 15 "$1" | sed -E 's/\x1b\[[0-9;]*m//g' | tr -d '\r'
}

# The fingerprint ignores what changes between two runs of the same failure: URLs, timings, the
# runner's temporary directory and colour codes.
fingerprint_of() {
  {
    printf '%s %s\n' "$package" "$version"
    for file in "$@"; do
      tail_of "$file"
    done
  } | sed -E \
      -e 's#https?://[^ ]+#<url>#g' \
      -e 's#[0-9]+(\.[0-9]+)?(ns|us|ms|s|m|h)( |$)#<time> #g' \
      -e 's#/(tmp|var/folders)/[^ ]+#<tmp>#g' \
    | sha256sum | cut -d' ' -f1
}

files=()
if [ -d "$logs" ]; then
  while IFS= read -r file; do files+=("$file"); done < <(find "$logs" -type f -name '*.txt' | sort)
fi
fingerprint="$(fingerprint_of "${files[@]}")"

{
  echo "The scheduled check of the published package failed."
  echo
  echo "- Package: \`${package}\` ${version}"
  echo "- Run: ${run}"
  echo
  if [ "${#files[@]}" -eq 0 ]; then
    echo "The failing job produced no output."
  else
    for file in "${files[@]}"; do
      name="$(basename "$(dirname "$file")")"
      if [ "$name" = "." ]; then name="$(basename "$file" .txt)"; fi
      echo "### ${name}"
      echo
      echo '```text'
      tail_of "$file"
      echo '```'
      echo
    done
  fi
  echo "<!-- published-check-fingerprint: ${fingerprint} -->"
} > "$body"

list="$(gh issue list --label "$label" --state open --limit 100 --json number,title)" || {
  echo "published-issue: gh issue list failed; check the published-check label" >&2
  exit 1
}
number="$(printf '%s' "$list" | jq -r --arg t "$title" '.[] | select(.title == $t) | .number' | head -n1)"

if [ -z "$number" ]; then
  gh issue create --title "$title" --body-file "$body" --label "$label"
  exit 0
fi

view="$(gh issue view "$number" --json body,comments)" || {
  echo "published-issue: gh issue view failed" >&2
  exit 1
}
last="$(printf '%s' "$view" | jq -r '(.comments[-1].body // .body) // ""')"
if printf '%s' "$last" | grep -Fq "<!-- published-check-fingerprint: ${fingerprint} -->"; then
  echo "published-issue: issue #${number} already reports this failure, no comment"
  exit 0
fi

gh issue comment "$number" --body-file "$body"
echo "published-issue: commented on issue #${number}"
