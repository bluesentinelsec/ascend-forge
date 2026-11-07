#!/usr/bin/env bash
# Verify a packaged ascend-forge.xcframework: slices, headers, version constants.
set -euo pipefail

xcframework="${1:-}"
expected_version="${2:-}"

if [[ -z "${xcframework}" || -z "${expected_version}" ]]; then
    echo "usage: $0 <ascend-forge.xcframework> <expected-version>" >&2
    exit 2
fi

device_library="${xcframework}/ios-arm64/libascend_forge-iphoneos.a"
simulator_library="${xcframework}/ios-arm64_x86_64-simulator/libascend_forge-iphonesimulator.a"

test -f "${xcframework}/Info.plist"
test -f "${device_library}"
test -f "${simulator_library}"

device_arches="$(lipo -archs "${device_library}")"
simulator_arches="$(lipo -archs "${simulator_library}")"
[[ " ${device_arches} " == *" arm64 "* ]]
[[ " ${simulator_arches} " == *" arm64 "* ]]
[[ " ${simulator_arches} " == *" x86_64 "* ]]

for identifier in ios-arm64 ios-arm64_x86_64-simulator; do
    header_root="${xcframework}/${identifier}/Headers"
    test -f "${header_root}/ascend_forge/version.hpp"
    test -f "${header_root}/ascend_forge/arena.h"
    test -f "${header_root}/ascend_forge/utf8.h"
    test -f "${header_root}/ascend_forge/result.h"
    test -f "${header_root}/ascend_forge/io.h"
    test -f "${header_root}/ascend_forge/json.h"
    test -f "${header_root}/ascend_forge/slice.h"
    test -f "${header_root}/ascend_forge/atomic.h"
    test -f "${header_root}/SDL3/SDL.h"
done

version_major="${expected_version%%.*}"
version_remainder="${expected_version#*.}"
version_minor="${version_remainder%%.*}"
version_patch="${version_remainder#*.}"
version_patch="${version_patch%%[-.]*}"
version_header="${xcframework}/ios-arm64/Headers/ascend_forge/version.hpp"
grep -q "kVersionMajor = ${version_major}" "${version_header}"
grep -q "kVersionMinor = ${version_minor}" "${version_header}"
grep -q "kVersionPatch = ${version_patch}" "${version_header}"

symbols_file="$(mktemp)"
visibility_file="$(mktemp)"
trap 'rm -f "${symbols_file}" "${visibility_file}"' EXIT
nm -gU "${device_library}" >"${symbols_file}"
grep -qE '[ST] _?_ZN12ascend_forge7Version' "${symbols_file}"
grep -qE 'T _Arena_Create$' "${symbols_file}"
grep -qE 'T _Utf8Builder_Append$' "${symbols_file}"
grep -qE 'T _Result_Fail$' "${symbols_file}"
grep -qE 'T _Io_ReadFile$' "${symbols_file}"
grep -qE 'T _Json_Parse$' "${symbols_file}"
grep -qE 'T _Slice_Append$' "${symbols_file}"
grep -qE 'T _AtomicU32_Load$' "${symbols_file}"
grep -qE 'T _SDL_malloc$' "${symbols_file}"
# yyjson stays in the archive for the linker and is not a public symbol.
# `nm -gU` still lists hidden symbols, so check the visibility flag.
nm -m "${device_library}" >"${visibility_file}"
if grep 'yyjson_' "${visibility_file}" | grep -v 'private external' | grep -v '(undefined)' | grep -q .; then
    echo "yyjson symbols are exported from the iOS archive" >&2
    exit 1
fi
grep -q 'private external _yyjson_read_opts' "${visibility_file}"

echo "Verified iOS XCFramework ${expected_version}"
echo "  device: ${device_arches}"
echo "  simulator: ${simulator_arches}"
