package com.example.myapplication;

import android.os.Bundle;
import android.view.View;

import android.content.pm.ActivityInfo;

import androidx.annotation.Keep;

import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;


public class MyNativeActivity extends android.app.NativeActivity {
	
	static {
		System.loadLibrary("mylua_android");
	}

    // Runtime-switchable UI policies
    public static final int UI_FIT_CLASSIC = 0; // system shrinks content area (no edge-to-edge)
    public static final int UI_FIT_MODERN  = 1; // edge-to-edge, bars visible; native must respect insets
    public static final int UI_IMMERSIVE   = 2; // edge-to-edge + hide system bars (swipe to reveal)

    private int uiMode = UI_FIT_MODERN;
	
	// Rotation bits must match the C++ header
	public static final int ROT_0   = 1 << 0;
	public static final int ROT_90  = 1 << 1;
	public static final int ROT_180 = 1 << 2;
	public static final int ROT_270 = 1 << 3;
	public static final int ROT_ALL = ROT_0 | ROT_90 | ROT_180 | ROT_270;

	private int allowedRotMask = ROT_ALL;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
		try {
			java.lang.reflect.Method m =
				MyNativeActivity.class.getDeclaredMethod("nativeOnInsetsChanged",
					int.class, int.class, int.class, int.class);
			android.util.Log.i("MyLuaApp", "nativeOnInsetsChanged method = " + m.toString());
		} catch (Throwable t) {
			android.util.Log.e("MyLuaApp", "Reflection failed", t);
		}

        super.onCreate(savedInstanceState);
		applyAllowedRotations();

        final View decor = getWindow().getDecorView();

        // Observe (not consume) system bar insets and forward to native
        ViewCompat.setOnApplyWindowInsetsListener(decor, (v, insets) -> {
            Insets sb = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            nativeOnInsetsChanged(sb.left, sb.top, sb.right, sb.bottom);
            return insets;
        });

        applyUiMode();
        ViewCompat.requestApplyInsets(decor);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            // Some devices/launchers reset bar state; re-apply when focus returns
            applyUiMode();
        }
    }

    // Called from native (JNI) to toggle modes at runtime
    public void setUiMode(int mode) {
		uiMode = mode;
		runOnUiThread(() -> {
			applyUiMode();
			ViewCompat.requestApplyInsets(getWindow().getDecorView());
		});
	}

    private void applyUiMode() {
        final View decor = getWindow().getDecorView();
        WindowInsetsControllerCompat c = new WindowInsetsControllerCompat(getWindow(), decor);

        if (uiMode == UI_IMMERSIVE) {
            //WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
            c.setSystemBarsBehavior(WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            c.hide(WindowInsetsCompat.Type.systemBars());
        } else if (uiMode == UI_FIT_CLASSIC) {
            //WindowCompat.setDecorFitsSystemWindows(getWindow(), true);
            c.show(WindowInsetsCompat.Type.systemBars());
        } else { // UI_FIT_MODERN
            //WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
            c.show(WindowInsetsCompat.Type.systemBars());
        }
    }

    @Keep
    private static native void nativeOnInsetsChanged(int l, int t, int r, int b);
	
	// Called from native (JNI)
	public void setAllowedRotations(int mask) {
		allowedRotMask = mask;
		runOnUiThread(() -> applyAllowedRotations());
	}
	
	private void applyAllowedRotations() {
		final int m = allowedRotMask;

		final int req;
		if (m == ROT_0) {
			req = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
		} else if (m == ROT_180) {
			req = ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT;
		} else if (m == ROT_90) {
			req = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
		} else if (m == ROT_270) {
			req = ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE;
		} else if (m == (ROT_0 | ROT_180)) {
			req = ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT;
		} else if (m == (ROT_90 | ROT_270)) {
			req = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;
		} else if (m == ROT_ALL) {
			req = ActivityInfo.SCREEN_ORIENTATION_FULL_SENSOR;
		} else {
			// Unhandled “mixed” masks: degrade gracefully
			req = ActivityInfo.SCREEN_ORIENTATION_FULL_SENSOR;
		}

		setRequestedOrientation(req);
	}

}

