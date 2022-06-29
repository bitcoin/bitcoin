package org.widecoincore.qt;

import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;

import org.qtproject.qt5.android.bindings.QtActivity;

import java.io.File;

public class WidecoinQtActivity extends QtActivity
{
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        final File widecoinDir = new File(getFilesDir().getAbsolutePath() + "/.widecoin");
        if (!widecoinDir.exists()) {
            widecoinDir.mkdir();
        }

        super.onCreate(savedInstanceState);
    }
}
