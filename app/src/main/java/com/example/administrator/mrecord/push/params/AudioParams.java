package com.example.administrator.mrecord.push.params;

import android.media.AudioFormat;

/**
 * Created by Administrator on 2017/5/14.
 */

public class AudioParams implements Params {
    public int sampleRate =44100;//todo音频降噪必须使用采样率16000 ,44100会产生杂音
//    public int channels= AudioFormat.CHANNEL_CONFIGURATION_STEREO;
    public int channels= AudioFormat.CHANNEL_CONFIGURATION_MONO;
}
