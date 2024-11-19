package com.example.hls;

import android.util.Log;
import android.app.Activity;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class App extends Activity {
    private MediaPlayer mediaPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Create a SurfaceView for video rendering
        SurfaceView surfaceView = new SurfaceView(this);
        setContentView(surfaceView);

        // Prepare the SurfaceHolder
        SurfaceHolder surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                Log.d("HLS", "surfaceCreated");
                try {
                    mediaPlayer = new MediaPlayer();
                    mediaPlayer.setDataSource(App.this, Uri.parse("https://livsho.com/live/stream.m3u8"));
                    mediaPlayer.setDisplay(holder);
                    mediaPlayer.setOnPreparedListener(MediaPlayer::start);
                    mediaPlayer.prepareAsync(); // Prepare asynchronously to avoid blocking the UI thread
                } catch (Exception e) {
                    Log.d("HLS", e.toString());
                }
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                Log.d("HLS", "surfaceChanged");
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                Log.d("HLS", "surfaceDestroyed");
                if (mediaPlayer != null) {
                    mediaPlayer.release();
                }
            }
        });
    }
}
