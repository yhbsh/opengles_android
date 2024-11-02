package com.example.activity;

import android.app.Activity;
import android.os.Bundle;
import android.content.Context;
import android.view.Gravity;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.SurfaceHolder;

public class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(new NativeSurfaceView(this));
    }
}

class NativeSurfaceView extends SurfaceView implements SurfaceHolder.Callback {
    static {
        System.loadLibrary("engine");
    }

    public NativeSurfaceView(Context context) {
        super(context);
        getHolder().addCallback(this);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        init(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        deinit();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        step(width, height);
    }

    private native void init(Surface surface);
    private native void step(int width, int height);
    private native void deinit();
}
