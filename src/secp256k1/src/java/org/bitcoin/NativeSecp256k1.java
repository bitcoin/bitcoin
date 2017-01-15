package org.bitcoin;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import com.google.common.base.Preconditions;


/**
 * This class holds native methods to handle ECDSA verification.
 * You can find an example library that can be used for this at
 * https://github.com/sipa/secp256k1
 */
public class NativeSecp256k1 {
    public static final boolean enabled;
    static {
        boolean isEnabled = true;
        try {
            System.loadLibrary("javasecp256k1");
        } catch (UnsatisfiedLinkError e) {
            isEnabled = false;
        }
        enabled = isEnabled;
    }
    
    private static ThreadLocal<ByteBuffer> nativeECDSABuffer = new ThreadLocal<ByteBuffer>();
    /**
     * Verifies the given secp256k1 signature in native code.
     * Calling when enabled == false is undefined (probably library not loaded)
     * 
     * @param data The data which was signed, must be exactly 32 bytes
     * @param signature The signature
     * @param pub The public key which did the signing
     */
    public static boolean verify(byte[] data, byte[] signature, byte[] pub) {
        Preconditions.checkArgument(data.length == 32 && signature.length <= 520 && pub.length <= 520);

        ByteBuffer byteBuff = nativeECDSABuffer.get();
        if (byteBuff == null) {
            byteBuff = ByteBuffer.allocateDirect(32 + 8 + 520 + 520);
            byteBuff.order(ByteOrder.nativeOrder());
            nativeECDSABuffer.set(byteBuff);
        }
        byteBuff.rewind();
        byteBuff.put(data);
        byteBuff.putInt(signature.length);
        byteBuff.putInt(pub.length);
        byteBuff.put(signature);
        byteBuff.put(pub);
        return secp256k1_ecdsa_verify(byteBuff) == 1;
    }

    /**
     * @param byteBuff signature format is byte[32] data,
     *        native-endian int signatureLength, native-endian int pubkeyLength,
     *        byte[signatureLength] signature, byte[pubkeyLength] pub
     * @returns 1 for valid signature, anything else for invalid
     */
    private static native int secp256k1_ecdsa_verify(ByteBuffer byteBuff);
}
