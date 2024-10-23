package com.example.game;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import android.view.Gravity;

public class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        TextView textView = new TextView(this);
        textView.setText("Hello, World!");
        textView.setTextSize(24);
        textView.setGravity(Gravity.CENTER);
        setContentView(textView);
    }
}
