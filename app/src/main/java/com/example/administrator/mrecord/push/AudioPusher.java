package com.example.administrator.mrecord.push;


import android.annotation.TargetApi;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Environment;
import android.util.Log;

import com.example.administrator.mrecord.AudioNativeUtils;
import com.example.administrator.mrecord.ThreadPoolFactory;
import com.example.administrator.mrecord.push.params.AudioParams;

import java.io.BufferedOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * Created by Administrator on 2017/5/14.
 */

public class AudioPusher extends Pusher<AudioParams> {

    private int bufferSize;
    private AudioRecord audioRecord;
    private AudioManager audioManager;
    private DataOutputStream dos;

    public AudioPusher(AudioParams parmas) {
        super (parmas);
    }

    @TargetApi(18)
    public AudioRecord getAudioRecord() {
        int audioEncoding = AudioFormat.ENCODING_PCM_16BIT;
        int audioSource = MediaRecorder.AudioSource.MIC;
        bufferSize = AudioRecord.getMinBufferSize (parmas.sampleRate, parmas.channels, AudioFormat.ENCODING_PCM_16BIT);
        audioRecord = new AudioRecord (audioSource, parmas.sampleRate,
                parmas.channels, audioEncoding, bufferSize);
        return audioRecord;
    }

    @Override
    public void prepare() {
        audioRecord = getAudioRecord ();
    }


    @Override
    public boolean statPush() {
        super.statPush ();
        String input = new File (Environment.getExternalStorageDirectory (), "record.pcm").getAbsolutePath ();
        //开通输出流到指定的文件
        try {
            dos = new DataOutputStream (new BufferedOutputStream (new FileOutputStream (new File (input))));
        } catch (FileNotFoundException e) {
            e.printStackTrace ();
        }
        audioRecord.startRecording ();
        encodeRecordData ();
        return true;
    }

    private void encodeRecordData() {
        ThreadPoolFactory.getNormalThreadPool ().execute (new Runnable () {
            @Override
            public void run() {
                int aacChannel = 1;
                if (parmas.channels == AudioFormat.CHANNEL_CONFIGURATION_STEREO) {
                    //AudioRecord 立体声 值为3 faac 立体声值为2
                    aacChannel = 2;
                }
//                AudioNativeUtils.prepare (parmas.sampleRate, aacChannel);

//                //（采样率*通道数*采样位数/8（比特换算成字节，一字节8比特） ）=字节数  网上有说法指2048个采样算一帧PCM
                int PCMBufferSize = 1024 * 16 / 8;
//                if (parmas.channels == AudioFormat.CHANNEL_CONFIGURATION_STEREO) {//双声道
//                    PCMBufferSize = PCMBufferSize * 2;//一秒的声音需要2k的存储空间
//                }
                int channelCount = 1;
                int frequency = parmas.sampleRate;//采样率
                int audioEncoding = AudioFormat.ENCODING_PCM_16BIT;
                int channelConfiguration = AudioFormat.CHANNEL_CONFIGURATION_MONO;
//                if (channelCount == 2) {
//                    channelConfiguration = AudioFormat.CHANNEL_CONFIGURATION_STEREO;
//                }
                PCMBufferSize = AudioRecord.getMinBufferSize (frequency, channelConfiguration, audioEncoding);
                Log.d ("AudioPusher", "PCMBufferSize:"+PCMBufferSize);

                byte[] buffer = new byte[PCMBufferSize];
                while (isPushing) {
                    int length = buffer.length;
                    int bufferReadResult = audioRecord.read (buffer, 0, length);
                    try {
                        dos.write (buffer, 0, bufferReadResult);
                    } catch (IOException e) {
                    }
                    //todo
//                    AudioNativeUtils.pushData (buffer, bufferReadResult);
                }
            }
        });
    }

    @Override
    public boolean stopPush() {
        super.stopPush ();
        audioRecord.stop ();
        return true;
    }

    @Override
    public boolean release() {
        if (audioManager != null) {
            audioManager.setMode (AudioManager.MODE_NORMAL);//通化模式
        }
        audioRecord.release ();
        AudioNativeUtils.release ();
        return true;
    }
}
