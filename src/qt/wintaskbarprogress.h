// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_WINTASKBARPROGRESS_H
#define BITCOIN_QT_WINTASKBARPROGRESS_H

#ifdef WIN32
#include <QWidget>

// Forward declarations for COM interfaces
struct ITaskbarList3;

/**
 * Native Windows taskbar progress indicator
 * Uses ITaskbarList3 COM interface for Windows 7+ compatibility
 * Works with both Qt5 and Qt6
 */
class WinTaskbarProgress
{
public:
    WinTaskbarProgress();
    ~WinTaskbarProgress();

    /** Set the window handle to attach progress to */
    void setWindow(QWidget* window);

    /** Set progress value (0-100) */
    void setValue(int value);

    /** Show or hide the progress indicator */
    void setVisible(bool visible);

    /** Reset progress state */
    void reset();

private:
    ITaskbarList3* m_taskbarList{nullptr};
    HWND m_winId{nullptr};
    bool m_initialized{false};

    void initializeTaskbar();
    void releaseTaskbar();
};

#endif // WIN32

#endif // BITCOIN_QT_WINTASKBARPROGRESS_H
