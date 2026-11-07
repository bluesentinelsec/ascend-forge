#!/usr/bin/env bash
# Build ascend-forge.xcframework: static device (arm64) and simulator
# (arm64/x86_64) libraries plus public headers and the generated version.h.
set -euo pipefail

repository_root="$(cd "$(dirname "$0")/.." && pwd)"
configuration="${1:-Release}"
configuration_lower="$(printf '%s' "${configuration}" | tr '[:upper:]' '[:lower:]')"
output_root="${2:-${repository_root}/build/ios/${configuration_lower}}"
deployment_target="${ASCEND_FORGE_IOS_DEPLOYMENT_TARGET:-13.0}"

case "${configuration}" in
    Debug|Release) ;;
    *)
        echo "Configuration must be Debug or Release (got '${configuration}')" >&2
        exit 2
        ;;
esac

if [[ "${output_root}" != /* ]]; then
    output_root="${repository_root}/${output_root}"
fi

version="$(tr -d '[:space:]' <"${repository_root}/VERSION" | sed 's/^v//;s/^V//;s/#.*//')"
device_build="${output_root}/iphoneos"
simulator_build="${output_root}/iphonesimulator"
combined_root="${output_root}/combined"
headers_root="${output_root}/headers"
xcframework="${output_root}/ascend-forge.xcframework"
archive="${output_root}/ascend-forge-ios-xcframework-${configuration_lower}-${version}.zip"

for tool in cmake xcodebuild xcrun libtool lipo ditto; do
    command -v "${tool}" >/dev/null 2>&1 || {
        echo "Required Apple build tool is missing: ${tool}" >&2
        exit 2
    }
done

cmake -E remove_directory "${output_root}"
cmake -E make_directory "${combined_root}"

configure_and_build() {
    local sdk="$1"
    local architectures="$2"
    local build_dir="$3"

    cmake -S "${repository_root}" -B "${build_dir}" -G Xcode \
        -DCMAKE_SYSTEM_NAME=iOS \
        -DCMAKE_OSX_SYSROOT="${sdk}" \
        -DCMAKE_OSX_ARCHITECTURES="${architectures}" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="${deployment_target}" \
        -DCMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET="${deployment_target}" \
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO \
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED=NO \
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY= \
        -DASCEND_FORGE_BUILD_APP=OFF \
        -DASCEND_FORGE_BUILD_TESTS=OFF \
        -DASCEND_FORGE_BUILD_BENCHMARKS=OFF \
        -DASCEND_FORGE_BUILD_PRIMITIVES=ON \
        -DASCEND_FORGE_WITH_CLI11=OFF \
        -DASCEND_FORGE_WITH_JSON=OFF \
        -DASCEND_FORGE_WITH_SPDLOG=OFF

    cmake --build "${build_dir}" --config "${configuration}" \
        --target "ascend-forge_lib" "ascend-forge_primitives" --parallel
}

merge_archives() {
    local build_dir="$1"
    local destination="$2"
    local core=""
    local primitives=""
    local sdl=""

    core="$(find "${build_dir}" -type f -name "libascend_forge.a" -path "*/${configuration}*" -print -quit)"
    primitives="$(find "${build_dir}" -type f -name "libascend_forge_primitives.a" -path "*/${configuration}*" -print -quit)"
    sdl="$(find "${build_dir}" -type f -name "libSDL3.a" -path "*/${configuration}*" -print -quit)"
    if [[ -z "${core}" || -z "${primitives}" || -z "${sdl}" ]]; then
        echo "Missing static archives in ${build_dir}" >&2
        echo "  core: ${core:-missing}" >&2
        echo "  primitives: ${primitives:-missing}" >&2
        echo "  SDL3: ${sdl:-missing}" >&2
        exit 1
    fi

    libtool -static -o "${destination}" "${core}" "${primitives}" "${sdl}"
}

configure_and_build iphoneos arm64 "${device_build}"
configure_and_build iphonesimulator 'arm64;x86_64' "${simulator_build}"

merge_archives "${device_build}" "${combined_root}/libascend_forge-iphoneos.a"
merge_archives "${simulator_build}" "${combined_root}/libascend_forge-iphonesimulator.a"

sdl_headers="$(find "${device_build}" -type d -path "*/sdl-src/include/SDL3" -print -quit)"
if [[ -z "${sdl_headers}" ]]; then
    echo "Missing fetched SDL3 public headers in ${device_build}" >&2
    exit 1
fi

cmake -E make_directory "${headers_root}/ascend_forge"
cmake -E copy_directory "${repository_root}/include/ascend_forge" "${headers_root}/ascend_forge"
cmake -E rm -f "${headers_root}/ascend_forge/.gitkeep"
cmake -E copy_directory "${sdl_headers}" "${headers_root}/SDL3"
cmake -E copy "${device_build}/generated/include/ascend_forge/version.h" \
    "${headers_root}/ascend_forge/version.h"

xcodebuild -create-xcframework \
    -library "${combined_root}/libascend_forge-iphoneos.a" \
    -headers "${headers_root}" \
    -library "${combined_root}/libascend_forge-iphonesimulator.a" \
    -headers "${headers_root}" \
    -output "${xcframework}"

ditto -c -k --norsrc --noextattr --keepParent "${xcframework}" "${archive}"

"${repository_root}/scripts/verify_ios_xcframework.sh" "${xcframework}" "${version}"

echo "XCFramework: ${xcframework}"
echo "Package: ${archive}"
