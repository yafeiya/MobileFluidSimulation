package com.example.mytest;

import androidx.appcompat.app.AppCompatActivity;
import com.google.androidgamesdk.GameActivity;

import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

//import com.example.mytest.databinding.ActivityMainBinding;


public class MainActivity extends GameActivity {

//    static {
//        Log.i("GameActivity", "Loading library...");
//        System.loadLibrary("game-activity");
//    }

    // Used to load the 'my_pbf' library on application startup.

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }
    static {
        //System.loadLibrary("android-game");
        System.loadLibrary("mytest");
    }

}