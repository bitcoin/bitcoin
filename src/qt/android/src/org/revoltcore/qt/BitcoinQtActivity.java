package org.bitcoincore.qt;

import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;

import org.qtproject.qt5.android.bindings.QtActivity;

import java.io.File;

public class RevoltQtActivity extends QtActivity
{
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        final File revoltDir = new File(getFilesDir().getAbsolutePath() + "/.bitcoin");
        if (!revoltDir.exists()) {
            revoltDir.mkdir();
        }

        super.onCreate(savedInstanceState);
    }
}
