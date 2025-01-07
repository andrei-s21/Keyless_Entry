package com.example.mycar;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.View;
import android.widget.Switch;

public class ActivitySettings extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings);

        ((Switch) findViewById(R.id.switch1)).setChecked(false);
        ((Switch)findViewById(R.id.switch2)).setChecked(getIntent().getBooleanExtra("keylessValue",true));
    }

    public void saveSettings(View view)
    {
        if(((Switch) findViewById(R.id.switch2)).isChecked())   BluetoothService.command=(byte)0xC7;
        else BluetoothService.command=(byte)0x9D;
        this.finish();
    }

    public void cancelSettings(View view)
    {
        this.finish();
    }
}