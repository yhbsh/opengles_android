package com.example.gridapp;

import android.app.Activity;
import android.graphics.Color;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.ViewGroup;
import android.widget.GridLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;

public class MainActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Get screen height to calculate item height
        DisplayMetrics displayMetrics = getResources().getDisplayMetrics();
        int itemHeight = 300;
        int paddingInPx = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 12, getResources().getDisplayMetrics());

        // Create ScrollView
        ScrollView scrollView = new ScrollView(this);
        scrollView.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        scrollView.setVerticalScrollBarEnabled(false); // Remove scrollbar

        // Create LinearLayout to hold the GridLayout
        LinearLayout container = new LinearLayout(this);
        container.setOrientation(LinearLayout.VERTICAL);
        container.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        // Create GridLayout
        GridLayout gridLayout = new GridLayout(this);
        gridLayout.setColumnCount(2);
        gridLayout.setPadding(paddingInPx, paddingInPx, paddingInPx, paddingInPx);
        gridLayout.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        // Populate grid with items
        for (int i = 0; i < 20; i++) { // Adjust number as needed
            ImageView item = new ImageView(this);
            GridLayout.LayoutParams params = new GridLayout.LayoutParams();
            params.width = (displayMetrics.widthPixels - 3 * paddingInPx) / 2; // Calculate width for 2 items per row with padding
            params.height = itemHeight;
            params.setMargins(paddingInPx / 2, paddingInPx / 2, paddingInPx / 2, paddingInPx / 2); // Set margins to avoid overlapping
            item.setLayoutParams(params);
            item.setBackgroundColor(Color.RED); // Set solid red background
            gridLayout.addView(item);
        }

        // Add GridLayout to LinearLayout
        container.addView(gridLayout);
        // Add LinearLayout to ScrollView
        scrollView.addView(container);
        // Set ScrollView as the content view
        setContentView(scrollView);
    }
}
