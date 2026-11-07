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

adb install -r "${apk}"
adb logcat -c
adb shell am force-stop "${package}"
adb shell am start -W -n "${activity}"

# Crash lines are easy to lose once the emulator log buffer wraps. Print them
# as soon as the test process is gone, and again if the wait expires.
relevant_logs() {
  adb logcat -d | grep -E "PASS:|FAIL:|ASCEND_FORGE_ANDROID_TESTS:|AndroidRuntime|FATAL EXCEPTION|UnsatisfiedLinkError|Fatal signal|dlopen|libascend_forge" || true
}

deadline=$((SECONDS + timeout_seconds))
while (( SECONDS < deadline )); do
  logs="$(adb logcat -d)"
  if grep -q "ASCEND_FORGE_ANDROID_TESTS: PASS" <<<"${logs}"; then
    relevant_logs
    exit 0
  fi
  if grep -q "ASCEND_FORGE_ANDROID_TESTS: FAIL" <<<"${logs}"; then
    relevant_logs >&2
    exit 1
  fi
  if grep -qE "Fatal signal|UnsatisfiedLinkError|FATAL EXCEPTION" <<<"${logs}"; then
    relevant_logs >&2
    echo "Android test process crashed" >&2
    exit 1
  fi
  if ! adb shell pidof "${package}" >/dev/null 2>&1; then
    relevant_logs >&2
    echo "Android test process exited before reporting a result" >&2
    exit 1
  fi
  sleep 1
done

relevant_logs >&2
echo "Timed out after ${timeout_seconds}s waiting for Android tests" >&2
exit 1
