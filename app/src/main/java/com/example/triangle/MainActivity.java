package com.example.triangle;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MainActivity extends Activity {
    static {
        System.loadLibrary("engine");
    }

    GLSurfaceView mView;

    public static native void init();

    public static native void resize(int width, int height);

    public static native void step();

    @Override
    protected void onCreate(Bundle bundle) {
        super.onCreate(bundle);

        setContentView(R.layout.activity_main);

        mView = findViewById(R.id.gl_surface_view);
        mView.setEGLConfigChooser(8, 8, 8, 0, 16, 0);
        mView.setEGLContextClientVersion(3);
        mView.setRenderer(new Renderer());
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
}
