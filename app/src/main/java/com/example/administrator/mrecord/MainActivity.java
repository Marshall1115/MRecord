package com.example.administrator.mrecord;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary ("native-lib");
    }

    private LiveEngine liveEngine;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate (savedInstanceState);
        setContentView (R.layout.activity_main);
        AppManager.getAppManager ().addActivity (this);
        liveEngine = LiveEngine.getInstace ();
        liveEngine.setVideoPath(new File (Environment.getExternalStorageDirectory (), "record.mp4").getAbsolutePath ());
        liveEngine.prepareCapture ((SurfaceView) findViewById (R.id.cv));

    }

    @Override
    protected void onDestroy() {
        super.onDestroy ();
        AppManager.getAppManager ().finishActivity (this);
    }

    private boolean isPushing;

    public void recordControl(View view) {
        Button button = (Button) view;
        if (!isPushing) {
            liveEngine.startRecord ();
            button.setText ("停止录制");
        } else {
            liveEngine.stopRecord ();
            button.setText ("开始录制");
        }
        isPushing = !isPushing;

    }
}
