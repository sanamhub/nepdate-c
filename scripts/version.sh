#!/usr/bin/env bash
# Prints the version in build.zig.zon, after checking the CMake project, the public header and the
# Conan recipes agree with it. The pre-release part lives in the manifests and the header, because a
# CMake project() holds numbers only.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"

version="$(sed -n 's/^[[:space:]]*\.version = "\(.*\)",/\1/p' "$root/build.zig.zon" | head -n1)"
if [ -z "$version" ]; then
  echo "version: build.zig.zon has no .version" >&2
  exit 1
fi
core="${version%%-*}"

found="$(sed -n 's/^[[:space:]]*project(nepdate-c VERSION \([0-9][0-9.]*\).*/\1/p' "$root/CMakeLists.txt" | head -n1)"
if [ "$found" != "$core" ]; then
  echo "version: CMakeLists.txt project version $found, want $core" >&2
  exit 1
fi

found="$(sed -n 's/^#define NEPDATE_VERSION_STRING "\(.*\)"/\1/p' "$root/include/nepdate.h" | head -n1)"
if [ "$found" != "$version" ]; then
  echo "version: include/nepdate.h NEPDATE_VERSION_STRING $found, want $version" >&2
  exit 1
fi

for recipe in "$root"/conan/*/conanfile.py; do
  found="$(sed -n 's/^    version = "\(.*\)"/\1/p' "$recipe" | head -n1)"
  if [ "$found" != "$version" ]; then
    echo "version: ${recipe#"$root"/} has $found, want $version" >&2
    exit 1
  fi
done

printf '%s\n' "$version"
