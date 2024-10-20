package com.example.gles3;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.view.View;
import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;

public class MainActivity extends Activity {
    private static class Renderer implements GLSurfaceView.Renderer {
        public void onDrawFrame(GL10 gl) {
            step();
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            resize(width, height);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            init();
        }
    }

    static {
        System.loadLibrary("engine");
    }

    public static native void init();

    public static native void resize(int width, int height);

    public static native void step();

    GLSurfaceView mView;

    @Override
    protected void onCreate(Bundle bundle) {
        super.onCreate(bundle);

        mView = new GLSurfaceView(this);
        mView.setEGLConfigChooser(8, 8, 8, 0, 16, 0);
        mView.setEGLContextClientVersion(3);
        mView.setRenderer(new Renderer());

        setContentView(mView);
        mView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mView.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        mView.onResume();
    }
}
