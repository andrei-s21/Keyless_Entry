
package com.example.mycar;

import android.util.Log;
import java.security.spec.AlgorithmParameterSpec;

import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

public class AES {

    private byte[] masterKey = {0x43, 0x52, 0x4f, 0x41, 0x5a, 0x49, 0x45, 0x52, 0x41, 0x20, 0x50, 0x45, 0x20, 0x4e, 0x49, 0x4c};
    private byte[] sessionKey = new byte[16]; // Inițial toate byte-urile sunt 0x00
    private byte[] iv = {0x4c, 0x45, 0x43, 0x20, 0x41, 0x43, 0x41, 0x53, 0x41, 0x4d, 0x41, 0x49, 0x4e, 0x45, 0x20, 0x50};

    private SecretKey currentKey;
    private AlgorithmParameterSpec ivSpec;

    public AES() {
        currentKey = new SecretKeySpec(masterKey, "AES");
        ivSpec = new IvParameterSpec(iv);
    }

    public void setSessionKey(byte[] newSessionKey) {
        System.arraycopy(newSessionKey, 0, sessionKey, 0, sessionKey.length);
    }

    public void useMasterKey() {
        currentKey = new SecretKeySpec(masterKey, "AES");
    }

    public void useSessionKey() {
        currentKey = new SecretKeySpec(sessionKey, "AES");
    }

    public byte[] encrypt(byte[] plain) {
        try {
            Cipher cipher = Cipher.getInstance("AES/CBC/NoPadding");
            cipher.init(Cipher.ENCRYPT_MODE, currentKey, ivSpec);
            return cipher.doFinal(plain);
        } catch (Exception e) {
            Log.e("AES", "Encryption error", e);
            return null;
        }
    }

    public byte[] decrypt(byte[] crypt) {
        try {
            Cipher cipher = Cipher.getInstance("AES/CBC/NoPadding");
            cipher.init(Cipher.DECRYPT_MODE, currentKey, ivSpec);
            return cipher.doFinal(crypt);
        } catch (Exception e) {
            Log.e("AES", "Decryption error", e);
            return null;
        }
    }
}
