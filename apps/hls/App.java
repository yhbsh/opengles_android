package com.example.hls;

import android.app.Activity;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Toast;

public class App extends Activity {
    private MediaPlayer mediaPlayer;
    private SurfaceView surfaceView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        surfaceView = new SurfaceView(this);
        setContentView(surfaceView);
        SurfaceHolder surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                initializeMediaPlayer(holder);
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {}

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                releaseMediaPlayer();
            }
        });
    }

    private void initializeMediaPlayer(SurfaceHolder holder) {
        releaseMediaPlayer(); // Ensure any previous instance is released
        mediaPlayer = new MediaPlayer();
        mediaPlayer.setDisplay(holder);
        mediaPlayer.setOnPreparedListener(MediaPlayer::start);
        mediaPlayer.setOnErrorListener((mp, what, extra) -> {
            String errorMessage = "Error playing stream: what=" + what + ", extra=" + extra;
            Toast.makeText(App.this, errorMessage, Toast.LENGTH_LONG).show();
            releaseMediaPlayer();
            return true;
        });
        mediaPlayer.setOnCompletionListener(mp -> releaseMediaPlayer());
        try {
            mediaPlayer.setDataSource(this, Uri.parse("http://192.168.1.187:8000/live/stream.m3u8"));
            mediaPlayer.prepareAsync();
        } catch (Exception e) {
            Toast.makeText(this, "Failed to load stream", Toast.LENGTH_SHORT).show();
            releaseMediaPlayer();
        }
    }

    private void releaseMediaPlayer() {
        if (mediaPlayer != null) {
            try {
                mediaPlayer.stop();
            } catch (Exception ignored) {}
            mediaPlayer.release();
            mediaPlayer = null;
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (mediaPlayer != null) {
            mediaPlayer.pause();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (mediaPlayer != null) {
            try {
                mediaPlayer.start();
            } catch (Exception ignored) {}
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        releaseMediaPlayer();
    }
}
