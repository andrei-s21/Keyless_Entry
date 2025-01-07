
package com.example.mycar;

import android.app.Service;
import android.bluetooth.BluetoothManager;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.PowerManager;
import android.util.Log;

public class BluetoothService extends Service {

    private Handler handler;
    private Runnable runnable;
    private BluetoothController controller = null;
    private PowerManager.WakeLock wakeLock;
    public static byte command = 0x00;

    public BluetoothService() {
    }

    @Override
    public void onCreate() {
        super.onCreate();
        PowerManager power = (PowerManager) getSystemService(Context.POWER_SERVICE);
        wakeLock = power.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "BluetoothService::WakeLock");
        wakeLock.acquire();

        handler = new Handler(Looper.getMainLooper());
        runnable = new Runnable() {
            @Override
            public void run() {
                if (controller.readyToSend) {
                    if (command != 0x00) {
                        controller.send(command);
                        command = 0x00;
                    } else {
                        controller.send((byte) 0xA4);
                    }
                }
                handler.postDelayed(this, 500);
            }
        };
        handler.postDelayed(runnable, 3000);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        controller = BluetoothController.getInstance((BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE));
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (wakeLock != null && wakeLock.isHeld()) {
            wakeLock.release();
        }
        handler.removeCallbacks(runnable);
        Log.d("BluetoothService", "Service destroyed");
    }
}
