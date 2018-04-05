package com.example.administrator.mrecord;

import android.hardware.Camera;
import android.view.SurfaceView;
import android.widget.Toast;

import com.example.administrator.mrecord.push.AudioPusher;
import com.example.administrator.mrecord.push.VideoPusher;
import com.example.administrator.mrecord.push.params.AudioParams;
import com.example.administrator.mrecord.push.params.VideoParams;
import com.facebook.soloader.SoLoader;

/**
 * Created by Administrator on 2017/5/14.
 */

public class LiveEngine {

    static {
        SoLoader.loadLibrary ("native-lib");
    }

    private static LiveEngine liveEngine = new LiveEngine ();
    private String videoPath;

    public LiveEngine() {
    }

    public static LiveEngine getInstace() {
        return liveEngine;
    }


    static AudioPusher ap = null;
    static VideoPusher vp = null;

    public void prepareCapture(SurfaceView view) {
        VideoParams videoParams = new VideoParams ();
        AudioParams audioParams = new AudioParams ();
        //todo setParams
        ap = new AudioPusher (audioParams);
//        videoParams.height = 720;
//        videoParams.width = 1280;
//        videoParams.height = 720;
//        videoParams.path = videoPath;
//        videoParams.width = 1280;
        videoParams.height = 480;
        videoParams.path = videoPath;
        videoParams.width = 800;
//        videoParams.height = view.getHeight();
//        videoParams.width = view.getWidth();
        videoParams.cameraId = Camera.CameraInfo.CAMERA_FACING_FRONT;//前置摄像头
        vp = new VideoPusher (videoParams);
        vp.setPreviewDisplay (view.getHolder ());

        ap.prepare ();
        vp.prepare ();
    }

    /**
     * 推流
     */
    public void startRecord() {
        prepareRecord (videoPath);
        if (!ap.isPushing () && !vp.isPushing ()) {
            Toast.makeText (UiUtils.getContext (), "startRecord", Toast.LENGTH_SHORT).show ();
            vp.statPush ();
            ap.statPush ();
        }

    }

    private native void prepareRecord(String videoPath);


    public static void stop() {
        vp.stopPush ();
        ap.stopPush ();
    }

    /**
     * 释放资源
     */
    public void release() {
        vp.stopPush ();
        vp.release ();

        ap.stopPush ();
        ap.release ();
    }

    public void stopRecord() {
        stop ();
    }

    public void setVideoPath(String videoPath) {
        this.videoPath = videoPath;
    }

//    public getAudio


}
