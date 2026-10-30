#!/usr/bin/env bash
# Checks a published release of nepdate-c the way a user gets it: downloads the source archive and
# the amalgamation archive from the GitHub release by URL, verifies SHA256SUMS, and builds a small
# consumer twice, once through CMake FetchContent from the source archive and once by compiling
# the amalgamation with the system C compiler. Both must print BS 2081-04-15 for AD 2024-07-30.
#
#   scripts/verify-published.sh 0.1.0-beta.1 [--manager cmake|amalgamation]
#   scripts/verify-published.sh latest --manager amalgamation
#   scripts/verify-published.sh latest --resolve
#   scripts/verify-published.sh latest --metadata
#
# "Published" is the GitHub release. latest is the newest non-draft release, pre-releases included
# while every release is one; the tag carries a v and the version here does not. --resolve prints
# the version alone and exits. --metadata reads the release metadata and sends HEAD requests to
# the two archives without downloading either, so it adds no download to the release.
#
# The archives are fetched anonymously from the public release URL; only the metadata reads use
# GITHUB_TOKEN, when it is set, to stay under the API rate limit.
set -euo pipefail

repository="sanamhub/nepdate-c"
api="https://api.github.com/repos/${repository}"
releases="https://github.com/${repository}/releases"
user_agent="nepdate-verify-published"

mode=install
version=""
manager=cmake
while [ $# -gt 0 ]; do
  case "$1" in
    --resolve) mode=resolve; shift ;;
    --metadata) mode=metadata; shift ;;
    --manager)
      manager="${2:?--manager needs cmake or amalgamation}"
      shift 2
      ;;
    -*)
      echo "verify-published: unknown argument '$1'"
      exit 2
      ;;
    *)
      version="$1"
      shift
      ;;
  esac
done
version="${version:-latest}"

api_get() {
  local authorization=()
  if [ -n "${GITHUB_TOKEN:-}" ]; then
    authorization=(-H "Authorization: Bearer ${GITHUB_TOKEN}")
  fi
  curl -fsS --retry 3 -H "Accept: application/vnd.github+json" -H "User-Agent: ${user_agent}" \
    "${authorization[@]}" "$1"
}

# The newest release a user can download: GitHub's release list is newest first, and a draft is
# not a release. Pre-releases are included because every 0.1.0-beta is one.
latest_version() {
  api_get "${api}/releases?per_page=5" \
    | jq -r '[.[] | select(.draft == false)][0].tag_name' \
    | sed -e 's/^v//'
}

if [ "$version" = latest ]; then
  version="$(latest_version)"
  if [ "$mode" != resolve ]; then
    echo "verify-published: the newest release is ${version}"
  fi
fi
if [ "$mode" = resolve ]; then
  printf '%s\n' "$version"
  exit 0
fi

if [[ ! "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z.-]+)?$ ]]; then
  echo "verify-published: '$version' is not a version like 0.1.0 or 0.1.0-beta.1"
  exit 2
fi

source_archive="nepdate-c-${version}.tar.gz"
amalgamation_archive="nepdate-c-${version}-amalgamation.tar.gz"
tag="v${version}"

# The daily check: the release still lists both archives and each URL still answers, without a
# download. A release that lost an asset or a tag that was deleted fails here within a day.
if [ "$mode" = metadata ]; then
  document="$(api_get "${api}/releases/tags/${tag}")"
  for name in "$source_archive" "$amalgamation_archive"; do
    url="$(printf '%s' "$document" | jq -r --arg n "$name" '.assets[] | select(.name == $n) | .browser_download_url')"
    if [ -z "$url" ]; then
      echo "verify-published: release ${tag} does not list ${name}"
      exit 1
    fi
    code="$(curl -sS -o /dev/null -w '%{http_code}' -I -L --retry 3 "$url")"
    if [ "$code" != 200 ]; then
      echo "verify-published: ${url} answered ${code}"
      exit 1
    fi
    echo "verify-published: ${name} is listed and its URL answers 200"
  done
  exit 0
fi

if [ "$manager" != cmake ] && [ "$manager" != amalgamation ]; then
  echo "verify-published: --manager must be cmake or amalgamation, not '${manager}'"
  exit 2
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
mkdir -p "$work/download"

echo "verify-published: downloading nepdate-c ${version} from ${releases}/tag/${tag}"
for name in "$source_archive" "$amalgamation_archive" SHA256SUMS; do
  curl -fsSL --retry 3 -o "$work/download/$name" "${releases}/download/${tag}/$name"
done
if command -v sha256sum > /dev/null; then
  (cd "$work/download" && sha256sum -c SHA256SUMS)
else
  (cd "$work/download" && shasum -a 256 -c SHA256SUMS)
fi

consumer="$work/consumer"
mkdir -p "$consumer"
cat > "$consumer/consumer.c" <<'EOF'
/* The README quick start plus one patro lookup, printed so the caller can compare. */
#include <nepdate.h>
#include <nepdate_patro.h>

#include <stdio.h>

int main(void) {
    nepdate_date date;
    nepdate_ymd bs;
    nepdate_patro_day_info info;
    nepdate_patro_name events[4];

    if (nepdate_from_ad(2024, 7, 30, &date) != NEPDATE_OK || nepdate_to_bs(date, &bs) != NEPDATE_OK) {
        return 1;
    }
    printf("BS %04d-%02d-%02d\n", bs.year, bs.month, bs.day);

    if (nepdate_from_bs(2081, 1, 1, &date) != NEPDATE_OK ||
        nepdate_patro_info(date, &info) != NEPDATE_OK) {
        return 1;
    }
    if (!info.is_public_holiday || info.event_count == 0) {
        return 1;
    }
    if (nepdate_patro_events(date, events, 4) == 0) {
        return 1;
    }
    printf("event: %.*s\n", (int)events[0].en_length, events[0].en);
    return 0;
}
EOF

want="BS 2081-04-15
event: New Year"
run_consumer() {
  local exe="$1"
  got="$("$exe" | tr -d '\r')"
  echo "$got"
  if [ "$got" != "$want" ]; then
    echo "verify-published: output differs; expected:"
    echo "$want"
    exit 1
  fi
}

if [ "$manager" = cmake ]; then
  sha256="$(awk -v n="$source_archive" '$2 == n { print $1 }' "$work/download/SHA256SUMS")"
  if [ -z "$sha256" ]; then
    echo "verify-published: SHA256SUMS has no line for ${source_archive}"
    exit 1
  fi
  cat > "$consumer/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.25)
project(nepdate_c_consumer LANGUAGES C)
include(FetchContent)
FetchContent_Declare(nepdate
  URL "${releases}/download/${tag}/${source_archive}"
  URL_HASH SHA256=${sha256})
FetchContent_MakeAvailable(nepdate)
add_executable(consumer consumer.c)
set_property(TARGET consumer PROPERTY C_STANDARD 11)
target_link_libraries(consumer PRIVATE nepdate-c::nepdate nepdate-c::patro)
EOF
  # CMake on Windows is a native program: it needs a C:/... path, not the /tmp/... of Git Bash.
  cmake_path() {
    if command -v cygpath > /dev/null; then cygpath -m "$1"; else printf '%s' "$1"; fi
  }
  cmake -S "$(cmake_path "$consumer")" -B "$(cmake_path "$consumer/build")" \
    -DCMAKE_BUILD_TYPE=Release \
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=$(cmake_path "$consumer/bin")"
  cmake --build "$(cmake_path "$consumer/build")" --config Release --parallel 2
  exe="$(find "$consumer/bin" -type f \( -name consumer -o -name consumer.exe \) | head -n 1)"
  if [ -z "$exe" ]; then
    echo "verify-published: the CMake build produced no consumer binary"
    exit 1
  fi
  run_consumer "$exe"
else
  mkdir -p "$work/amalgamation"
  tar -xzf "$work/download/$amalgamation_archive" -C "$work/amalgamation"
  cp "$consumer/consumer.c" "$work/amalgamation/consumer.c"
  compiler="${CC:-cc}"
  echo "verify-published: compiling the amalgamation with ${compiler}"
  (cd "$work/amalgamation" && "$compiler" -std=c11 -O2 -I. -o consumer \
    consumer.c nepdate.c nepdate_patro.c)
  run_consumer "$work/amalgamation/consumer"
fi

echo "verify-published: ok, nepdate-c ${version} from the release, built with ${manager}"
