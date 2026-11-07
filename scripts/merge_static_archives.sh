#!/usr/bin/env bash
# Replace dest with one static archive containing every object from the
# following archives. Member names are rewritten so two objects that share
# a basename both survive.
set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "usage: $0 <destination.a> <archive.a>..." >&2
    exit 2
fi

destination="$1"
shift

archiver="ar"
if command -v emar >/dev/null 2>&1; then
    archiver="emar"
fi

absolute_path() {
    local path="$1"
    local dir
    dir="$(cd "$(dirname "${path}")" && pwd)"
    printf '%s/%s\n' "${dir}" "$(basename "${path}")"
}

destination="$(absolute_path "${destination}")"

tmp="$(mktemp -d)"
trap 'rm -rf "${tmp}"' EXIT

index=0
objects=()
for archive in "$@"; do
    archive="$(absolute_path "${archive}")"
    if [[ ! -f "${archive}" ]]; then
        echo "Static archive not found: ${archive}" >&2
        exit 1
    fi
    extract="${tmp}/src${index}"
    mkdir -p "${extract}"
    (cd "${extract}" && "${archiver}" x "${archive}")
    while IFS= read -r object; do
        renamed="${tmp}/m${index}_$(basename "${object}")"
        cp "${object}" "${renamed}"
        objects+=("${renamed}")
    done < <(find "${extract}" -type f \( -name '*.o' -o -name '*.obj' \) -print)
    index=$((index + 1))
done

if [[ ${#objects[@]} -eq 0 ]]; then
    echo "No objects found to merge into ${destination}" >&2
    exit 1
fi

rm -f "${destination}"
"${archiver}" rcs "${destination}" "${objects[@]}"
