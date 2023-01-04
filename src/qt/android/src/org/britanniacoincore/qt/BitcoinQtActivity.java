package org.britanniacoincore.qt;

import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;

import org.qtproject.qt5.android.bindings.QtActivity;

import java.io.File;

public class BritanniaCoinQtActivity extends QtActivity
{
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        final File britanniacoinDir = new File(getFilesDir().getAbsolutePath() + "/.britanniacoin");
        if (!britanniacoinDir.exists()) {
            britanniacoinDir.mkdir();
        }

        super.onCreate(savedInstanceState);
    }
}
