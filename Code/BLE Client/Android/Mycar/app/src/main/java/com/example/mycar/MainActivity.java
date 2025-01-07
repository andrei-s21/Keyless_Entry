
package com.example.mycar;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    public static MainActivity mna;
    public static ImageView bluetoothIcon = null;
    public static ImageView lightsIcon = null;
    public static ImageView lockIcon = null;
    public static ImageView engineIcon = null;
    public static TextView gearText = null;
    public static TextView engineText = null;
    public static TextView voltageText = null;
    public static TextView tempText = null;

    public static boolean keylessEnabled = true;

    public void setData(byte[] data) {
        String voltage = (data[0] / 10) + "." + (data[0] % 10) + "V";
        voltageText.setText(voltage);

        keylessEnabled = (data[2] & 0x08) != 0;

        lockIcon.setImageResource((data[2] & 0x04) != 0 ? R.drawable.lock_3 : R.drawable.unlock_2);

        if ((data[2] & 0x02) != 0) {
            engineIcon.setImageResource(R.drawable.engine_on);
            engineText.setText("Engine On");
        } else {
            engineIcon.setImageResource(R.drawable.engine_off_2);
            engineText.setText("Engine Off");
        }

        gearText.setText((data[2] & 0x10) != 0 ? "In Gear" : "Not in Gear");

        lightsIcon.setImageResource((data[2] & 0x01) != 0 ? R.drawable.light_on : R.drawable.lights);
    }

    private void setDisplayReferences() {
        bluetoothIcon = findViewById(R.id.statusConnection);
        gearText = findViewById(R.id.valueGear);
        engineText = findViewById(R.id.valueEngine);
        lightsIcon = findViewById(R.id.statusLights);
        engineIcon = findViewById(R.id.statusEngine);
        lockIcon = findViewById(R.id.statusLock);
        voltageText = findViewById(R.id.valueVoltage);
        tempText = findViewById(R.id.valueTemp);
    }

    public void setViewsForDisconnect() {
        bluetoothIcon.setImageResource(R.drawable.bluetooth_disconnect_2);
        gearText.setText("");
        engineText.setText("");
        lightsIcon.setImageDrawable(null);
        engineIcon.setImageDrawable(null);
        lockIcon.setImageDrawable(null);
        voltageText.setText("");
        tempText.setText("");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mna = this;
        setDisplayReferences();

        startService(new Intent(MainActivity.mna, BluetoothService.class));
    }

    public void openSettings(View view) {
        startActivity(new Intent(this, ActivitySettings.class).putExtra("keylessValue", keylessEnabled));
    }

    public void light(View view) {
        BluetoothService.command = (byte) 0xAA;
    }

    public void unlock(View view) {
        BluetoothService.command = (byte) 0x47;
    }

    public void lock(View view) {
        BluetoothService.command = (byte) 0xFC;
    }

    public void startStop(View view) {
        BluetoothService.command = (byte) 0x1B;
    }
}
