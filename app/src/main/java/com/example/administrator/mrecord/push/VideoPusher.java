package com.example.administrator.mrecord.push;

import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.util.Log;
import android.view.SurfaceHolder;

import com.example.administrator.mrecord.AppManager;
import com.example.administrator.mrecord.UiUtils;
import com.example.administrator.mrecord.push.params.VideoParams;
import com.example.administrator.mrecord.utils.PicutureNativeUtils;

import java.io.IOException;

/**
 * Created by Administrator on 2017/5/14.
 */

public class VideoPusher extends Pusher<VideoParams> {
    private SurfaceHolder holder;
    private Camera mCamera;

    public VideoPusher(VideoParams params) {
        super (params);
        init ();
    }

    private void init() {
        mCamera = getCameraInstance ();
    }

    @Override
    public void prepare() {
        //提示：x264宽高相反
//        PicutureNativeUtils.prepare (480,800 );
        holder.addCallback (new SurfaceHolder.Callback () {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                try {
                    startPreview ();
                } catch (IOException e) {
                    //todo 释放资源
                    e.printStackTrace ();
                    throw new RuntimeException ();
                }
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                mCamera.startPreview ();
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                release ();
            }
        });
    }

    private void startPreview() throws IOException {
        adjustCameraOrientation ();
        Log.d ("VideoPusher", "startPreview");
        mCamera.setPreviewDisplay (holder);
        Camera.Parameters parameters = mCamera.getParameters ();
        parameters.setPreviewSize (parmas.width,parmas.height);
        parameters.setPreviewFormat (ImageFormat.NV21);
        mCamera.setParameters (parameters);
//        mCamera.setDisplayOrientation (90);
        mCamera.addCallbackBuffer (new byte[parmas.height * parmas.width * 3/2]);
        mCamera.setPreviewCallbackWithBuffer (new Camera.PreviewCallback () {
            @Override
            public void onPreviewFrame(byte[] data, Camera camera) {
                mCamera.addCallbackBuffer (data);
                if (isPushing ()) {
                    // todo 视频推流
                    PicutureNativeUtils.pushData (data);
                }
            }
        });
//        mCamera.addCallbackBuffer (new byte[parmas.height * parmas.width * 4]);
    }

    private void adjustCameraOrientation() {
        Camera.CameraInfo mCameraInfo = new Camera.CameraInfo ();
        Camera.getCameraInfo (parmas.cameraId, mCameraInfo);
        Log.d ("VideoPusher", "handle setting camera orientation, mCameraInfo.facing:" + mCameraInfo.facing);
        int degrees = UiUtils.getDeviceRotationDegree (AppManager.getAppManager ().currentActivity ());
        int result;
        if (mCameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            result = (mCameraInfo.orientation + degrees) % 360;
            result = (360 - result) % 360;  // compensate the mirror
        } else {  // back-facing
            result = (mCameraInfo.orientation - degrees + 360) % 360;
        }

        mCamera.setDisplayOrientation (result);

    }

    private void stopVideo() {
        if (null == mCamera)
            return;
        try {
            mCamera.stopPreview ();
            mCamera.setPreviewDisplay (null);
            mCamera.setPreviewCallbackWithBuffer (null);
            mCamera.release ();
        } catch (IOException e) {
            e.printStackTrace ();
            return;
        }
        mCamera = null;
    }

    @Override
    public boolean statPush() {
        return super.statPush ();
    }

    @Override
    public boolean stopPush() {
        return super.stopPush ();
    }

    @Override
    public boolean release() {
        Log.d ("VideoPusher", "release ");
        stopVideo ();
        return true;
    }

    public void setPreviewDisplay(SurfaceHolder holder) {
        this.holder = holder;
    }

    /**
     * A safe way to get an instance of the Camera object.
     */
    public Camera getCameraInstance() {
        Camera c = null;
        try {
            c = Camera.open (parmas.cameraId); // attempt to get a Camera instance
        } catch (Exception e) {
            e.printStackTrace ();
            // Camera is not available (in use or does not exist)
        }
        return c; // returns null if camera is unavailable
    }
}
