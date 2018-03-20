package com.example.administrator.mrecord;

import android.app.Application;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;

import com.facebook.soloader.SoLoader;

public class AppContext extends Application {
    private static AppContext instance;
    private static Context context;
    private static Handler mHandler;
    public static Context getContext() {
        return context;
    }
    public static Handler getHandler() {
        return mHandler;
    }
    @Override
    public void onCreate() {
        instance = this;
        this.context = getApplicationContext();
        super.onCreate();
        mHandler = new Handler ();
        SoLoader.init (this, false);
    }

    public static boolean isMainThread() {
        return Looper.myLooper() == Looper.getMainLooper();
    }
    public static Application instance() {
        return instance;
    }
}
