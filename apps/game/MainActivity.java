package com.example.game;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;

public class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        ListView listView = new ListView(this);
        listView.setAdapter(new MainListAdapter());
        setContentView(listView);
    }

    class MainListAdapter extends BaseAdapter {
        private String[] data;

        MainListAdapter() {
            data = new String[100];
            for (int i = 0; i < 100; i++) {
                data[i] = "Item " + (i + 1);
            }
        }

        @Override
        public int getCount() {
            return data.length;
        }

        @Override
        public Object getItem(int position) {
            return data[position];
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            TextView textView;
            if (convertView == null) {
                textView = new TextView(MainActivity.this);
                textView.setPadding(20, 20, 20, 20);
            } else {
                textView = (TextView) convertView;
            }

            // Retrieve item from data array
            String item = data[position];

            // Check if item is null before setting text
            if (item != null) {
                textView.setText(item);
            } else {
                textView.setText("Unknown Item"); // Provide a fallback for null values
            }
            return textView;
        }
    }
}
