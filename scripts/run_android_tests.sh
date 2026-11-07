#!/usr/bin/env bash
set -euo pipefail

apk="${1:-android/test-app/build/outputs/apk/release/test-app-release.apk}"
package="com.example.ascendforge.test"
activity="${package}/.TestActivity"
timeout_seconds="${ASCEND_FORGE_ANDROID_TEST_TIMEOUT:-120}"

if [[ ! -f "${apk}" ]]; then
  echo "Android test APK not found: ${apk}" >&2
  exit 1
fi

adb install -r "${apk}" >/dev/null
adb logcat -c
adb shell am force-stop "${package}"
adb shell am start -W -n "${activity}" >/dev/null

deadline=$((SECONDS + timeout_seconds))
while (( SECONDS < deadline )); do
  logs="$(adb logcat -d)"
  if grep -q "ASCEND_FORGE_ANDROID_TESTS: PASS" <<<"${logs}"; then
    printf '%s\n' "${logs}" | grep -E "PASS:|FAIL:|ASCEND_FORGE_ANDROID_TESTS:"
    exit 0
  fi
  if grep -q "ASCEND_FORGE_ANDROID_TESTS: FAIL" <<<"${logs}"; then
    printf '%s\n' "${logs}" | grep -E "PASS:|FAIL:|ASCEND_FORGE_ANDROID_TESTS:" >&2
    exit 1
  fi
  sleep 1
done

adb logcat -d | grep -E "PASS:|FAIL:|ASCEND_FORGE_ANDROID_TESTS:" >&2 || true
echo "Timed out after ${timeout_seconds}s waiting for Android tests" >&2
exit 1
