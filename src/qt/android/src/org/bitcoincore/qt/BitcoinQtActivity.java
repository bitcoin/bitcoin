package org.bitcoincore.qt;

import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;

import org.qtproject.qt5.android.bindings.QtActivity;

import java.io.File;

public class BitcoinQtActivity extends QtActivity
{

    final private int REQUEST_CODE_ASK_MULTIPLE_PERMISSIONS = 124;
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        final File bitcoinDir = new File(getFilesDir().getAbsolutePath() + "/.bitcoin");
        if (!bitcoinDir.exists()) {
            bitcoinDir.mkdir();
        }

        try {
            Os.setenv("QT_QPA_PLATFORM", "android", true);
        } catch (ErrnoException e) {
            e.printStackTrace();
        }

        super.onCreate(savedInstanceState);

        // Marshmallow+ Permission APIs
        if (Build.VERSION.SDK_INT >= 23) {
             fuckMarshMallow();
         }
    }

@Override
public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
    switch (requestCode) {
        case REQUEST_CODE_ASK_MULTIPLE_PERMISSIONS: {
            Map<String, Integer> perms = new HashMap<String, Integer>();
            perms.put(Manifest.permission.ACCESS_FINE_LOCATION, PackageManager.PERMISSION_GRANTED);
            for (int i = 0; i < permissions.length; i++)
                perms.put(permissions[i], grantResults[i]);
            if (perms.get(Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED

            ) {
                 Toast.makeText(BitcoinQtActivity.this, "All Permission GRANTED!", Toast.LENGTH_SHORT).show();
            } else {
                 Toast.makeText(BitcoinQtActivity.this, "One or More Permissions are DENIED Exiting App!", Toast.LENGTH_SHORT).show();
                 finish();
            }
        }
        break;
        default:
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }
}

@TargetApi(Build.VERSION_CODES.M)
private void fuckMarshMallow() {
    List<String> permissionsNeeded = new ArrayList<String>();
    final List<String> permissionsList = new ArrayList<String>();
    if (!addPermission(permissionsList, Manifest.permission.ACCESS_FINE_LOCATION))
        permissionsNeeded.add("Show Location");
    if (permissionsList.size() > 0) {
        if (permissionsNeeded.size() > 0) {
            String message = "App need access to " + permissionsNeeded.get(0);
            for (int i = 1; i < permissionsNeeded.size(); i++)
                message = message + ", " + permissionsNeeded.get(i);
            requestPermissions(permissionsList.toArray(new String[permissionsList.size()]),
                                    REQUEST_CODE_ASK_MULTIPLE_PERMISSIONS);
            return;
        }
        requestPermissions(permissionsList.toArray(new String[permissionsList.size()]), REQUEST_CODE_ASK_MULTIPLE_PERMISSIONS);
        return;
    }
    Toast.makeText(BitcoinQtActivity.this, "No new Permission Required - Launching Bitcoin Core.", Toast.LENGTH_SHORT).show();
}

@TargetApi(Build.VERSION_CODES.M)
private boolean addPermission(List<String> permissionsList, String permission) {
    if (checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) {
        permissionsList.add(permission);
        if (!shouldShowRequestPermissionRationale(permission))
            return false;
    }
    return true;
}

}
