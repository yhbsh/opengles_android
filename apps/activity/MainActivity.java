package com.example.activity;

import android.app.Activity;
import android.os.Bundle;
import android.view.Gravity;
import android.widget.TextView;

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
