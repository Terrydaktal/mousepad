#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)
[[ $# -ge 1 && $# -le 2 ]] || exit 2
header=$1
template=${2:-"$root/fork-version.h.in"}
[[ -r $template ]] || {
	printf 'Missing version template: %s\n' "$template" >&2
	exit 1
}
output=$(dirname -- "$header")
version=$(bash "$root/scripts/build-version.sh" "$output")
commit=$(cat -- "$output/SOURCE-COMMIT")
temporary=$(mktemp "$header.XXXXXX")
trap 'rm -f -- "$temporary"' EXIT
sed -e "s/@REVISION@/$commit/g" -e "s/@VERSION_FULL@/$version/g" "$template" >"$temporary"
mv -f -- "$temporary" "$header"
printf '%s\n' "$version"
