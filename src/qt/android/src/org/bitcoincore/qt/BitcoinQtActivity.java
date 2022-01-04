package org.syscoincore.qt;

import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;

import org.qtproject.qt5.android.bindings.QtActivity;

import java.io.File;

public class SyscoinQtActivity extends QtActivity
{
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        final File syscoinDir = new File(getFilesDir().getAbsolutePath() + "/.syscoin");
        if (!syscoinDir.exists()) {
            syscoinDir.mkdir();
        }

        super.onCreate(savedInstanceState);
    }
}
