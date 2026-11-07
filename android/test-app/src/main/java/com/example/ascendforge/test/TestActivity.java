package com.example.ascendforge.test;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;

import org.libsdl.app.SDL;

public final class TestActivity extends Activity {
    private static final String TAG = "ascend-forge-android-test";

    static {
        System.loadLibrary("ascend_forge_android_test");
    }

    private static native int runNativeTests();

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        SDL.setContext(this);
        Thread runner = new Thread(() -> {
            int failures = runNativeTests();
            if (failures == 0) {
                Log.i(TAG, "ASCEND_FORGE_ANDROID_TESTS: PASS");
            } else {
                Log.e(TAG, "ASCEND_FORGE_ANDROID_TESTS: FAIL (" + failures + " failures)");
            }
            finishAndRemoveTask();
        }, "ascend-forge-native-tests");
        runner.start();
    }
}
