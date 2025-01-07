
package com.example.mycar;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.pm.PackageManager;
import android.util.Log;
import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;

import java.util.List;
import java.util.Random;
import java.util.UUID;

public class BluetoothController {

    private static BluetoothController instance;
    private BluetoothGatt btGatt;
    private BluetoothGattCharacteristic characteristicSend;
    private BluetoothGattCharacteristic characteristicRecive;
    private AES aes;
    private byte[] msg;
    private Random random;
    private byte[] TOKEN = new byte[4];
    private byte[] TOKENC = new byte[4];
    public boolean isConnected = false;
    public boolean readyToSend = false;

    private final BluetoothGattCallback bleutoothGattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            Log.d("data", String.valueOf(status));
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                MainActivity.bluetoothIcon.setImageResource(R.drawable.bluetooth_connected_2);
                if (ActivityCompat.checkSelfPermission(MainActivity.mna, android.Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                    Log.e("data", "No permissions");
                    return;
                }
                Log.d("data", "Connected");
                isConnected = true;
                btGatt.discoverServices();
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                isConnected = false;
                readyToSend = false;
                MainActivity.mna.setViewsForDisconnect();
            }
        }

        @SuppressLint("MissingPermission")
        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                getServices();
            } else if (status == BluetoothGatt.GATT_FAILURE) {
                btGatt.disconnect();
                isConnected = false;
            }
        }

        @Override
        public void onCharacteristicChanged(@NonNull BluetoothGatt gatt, @NonNull BluetoothGattCharacteristic characteristic, @NonNull byte[] value) {
            if (characteristic == characteristicRecive) {
                processHandshake(value);
            }
        }
    };

    private void processHandshake(byte[] data) {
        if (data[0] == (byte) 0xA4) {

            System.arraycopy(data, 2, TOKEN, 0, 4);
            System.arraycopy(data, 6, TOKENC, 0, 4);

            byte[] sessionKey = new byte[16];
            System.arraycopy(TOKEN, 0, sessionKey, 0, 4);
            System.arraycopy(TOKENC, 0, sessionKey, 4, 4);
            aes.setSessionKey(sessionKey);
            aes.useSessionKey();

            byte[] response = new byte[16];
            response[0] = 0x21;
            response[1] = (byte) 0xB3;
            System.arraycopy(TOKENC, 0, response, 2, 4);
            System.arraycopy(TOKEN, 0, response, 6, 4);
            response[10] = (byte) random.nextInt(255);
            response[15] = calculateChecksum(response);

            byte[] crypt = aes.encrypt(response);
            sendEncrypted(crypt);
        } else if (data[0] == (byte) 0xC2) {
            readyToSend = validateResponse(data);
        } else {
            Log.e("Handshake", "Unexpected data received.");
        }
    }

    private boolean validateResponse(byte[] data) {

        if (data[15] != calculateChecksum(data)) {
            Log.e("Handshake", "Invalid checksum.");
            return false;
        }
        for (int i = 2; i < 6; i++) {
            if (data[i] != TOKENC[i - 2]) {
                Log.e("Handshake", "Invalid TOKENC.");
                return false;
            }
        }
        return true;
    }

    public void send(byte command) {
        msg = new byte[16];
        msg[0] = (byte) 0x21;
        msg[1] = TOKEN[0];
        msg[2] = TOKEN[1];
        msg[3] = TOKEN[2];
        msg[4] = TOKEN[3];
        msg[5] = (byte) random.nextInt(255);
        msg[6] = command;
        for (int i = 7; i < 15; i++) {
            msg[i] = 0x00;
        }
        msg[15] = calculateChecksum(msg);

        byte[] crypt = aes.encrypt(msg);
        sendEncrypted(crypt);
    }

    private void sendEncrypted(byte[] crypt) {
        if (crypt != null && ActivityCompat.checkSelfPermission(MainActivity.mna, android.Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED) {
            btGatt.writeCharacteristic(characteristicSend, crypt, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
        } else {
            Log.e("BluetoothController", "Failed to send encrypted data.");
        }
    }

    private byte calculateChecksum(byte[] data) {
        byte checksum = 0;
        for (int i = 0; i < 15; i++) {
            checksum ^= data[i];
        }
        return checksum;
    }

    public static BluetoothController getInstance(BluetoothManager manager) {
        if (BluetoothController.instance == null) {
            BluetoothController.instance = new BluetoothController();
            BluetoothController.instance.start(manager);
        }
        return instance;
    }

    private void start(BluetoothManager manager) {
        aes = new AES();
        random = new Random();

        BluetoothDevice device = manager.getAdapter().getRemoteDevice("0C:DC:7E:69:C8:A2");
        if (device == null) {
            Log.e("data", "Device is null");
            return;
        }
        if (ActivityCompat.checkSelfPermission(MainActivity.mna, android.Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            Log.e("data", "No permissions to connect");
            return;
        }
        btGatt = device.connectGatt(MainActivity.mna, true, instance.bleutoothGattCallback);
    }

    @SuppressLint("MissingPermission")
    private void getServices() {
        List<BluetoothGattService> list = btGatt.getServices();
        if (list == null) {
            Log.e("data", "Service list is null");
            return;
        }
        BluetoothGattService service = null;
        for (BluetoothGattService serv : list) {
            if (serv.getUuid().toString().equals("5daea2b4-1912-4839-8c9d-7657f074fa84")) {
                service = serv;
                break;
            }
        }
        if (service == null) {
            Log.e("data", "Service not found");
            disconnect();
            return;
        }
        characteristicSend = service.getCharacteristic(UUID.fromString("f61ad6ec-247b-44e9-98e1-f2a61b9836ea"));
        characteristicRecive = service.getCharacteristic(UUID.fromString("df847cb0-69aa-43e1-8aa9-1fad496c60b5"));
        btGatt.setCharacteristicNotification(characteristicRecive, true);
        BluetoothGattDescriptor desc = characteristicRecive.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"));
        btGatt.writeDescriptor(desc, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
        readyToSend = true;
    }

    private void disconnect() {
        if (ActivityCompat.checkSelfPermission(MainActivity.mna, android.Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            Log.e("data", "No permissions to disconnect");
            return;
        }
        btGatt.disconnect();
    }
}
