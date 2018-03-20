package com.example.administrator.mrecord.utils;

import android.content.Context;
import android.content.res.Resources;
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Toast;

import com.example.administrator.mrecord.AppContext;


/**
 * user: marshall
 * date: 2016/4/21.
 * fixme
 * <p>
 */
public class UiUtils {

    public static Context getContext() {
        return AppContext.getContext ();
    }

    public static View inflate(int resId) {
        return View.inflate (AppContext.getContext (), resId, null);
    }

    public static View inflate(int resId, ViewGroup parent) {
        return LayoutInflater.from (parent.getContext ())
                .inflate (resId, parent, false);
    }

    public static void showToast(final String msg) {
        HandlerUtils.postTaskSafely (new Runnable () {
                                         @Override
                                         public void run() {
                                             Toast.makeText (getContext (), msg, Toast.LENGTH_SHORT).show ();
                                         }
                                     }
        );
    }

    public static String[] getStringArray(int id) {
        String[] str = getResources ().getStringArray (id);
        return str;
    }

    public static Resources getResources() {return getContext ().getResources ();}

    public static int getDimensionPixelSize(int id) {
        int size = getResources ().getDimensionPixelSize (id);
        return size;
    }

    public static String getString(int id) {
        return getResources ().getString (id);
    }

    public static int getColor(int color) {
        return getResources ().getColor (color);
    }

    public static int getDisplayDefaultRotation(Context ctx) {
        WindowManager windowManager =  (WindowManager) ctx.getSystemService(Context.WINDOW_SERVICE);
        return windowManager.getDefaultDisplay().getRotation();
    }
    public static int getDeviceRotationDegree(Context ctx) {
        switch (getDisplayDefaultRotation(ctx)) {
            // normal portrait
            case Surface.ROTATION_0:
                return 0;
            // expected landscape
            case Surface.ROTATION_90:
                return 90;
            // upside down portrait
            case Surface.ROTATION_180:
                return 180;
            // "upside down" landscape
            case Surface.ROTATION_270:
                return 270;
        }
        return 0;
    }
}
