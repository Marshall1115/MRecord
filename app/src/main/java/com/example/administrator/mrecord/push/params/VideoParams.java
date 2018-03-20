package com.example.administrator.mrecord.push.params;

/**
 * Created by Administrator on 2017/5/14.
 */

public class VideoParams implements Params {
    public int height;
    public int width;
    public String path;
    /**
     * @see android.hardware.Camera.CameraInfo
     *  CAMERA_FACING_BACK and CAMERA_FACING_FRONT
     */
    public int cameraId;
}
