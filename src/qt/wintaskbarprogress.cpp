// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/wintaskbarprogress.h>

#if defined(Q_OS_WIN)

#include <QWidget>
#include <QWindow>

#include <windows.h>
#include <shobjidl.h>

WinTaskbarProgress::WinTaskbarProgress()
{
    // COM will be initialized by Qt or the application
}

WinTaskbarProgress::~WinTaskbarProgress()
{
    releaseTaskbar();
}

void WinTaskbarProgress::setWindow(QWidget* window)
{
    if (!window) {
        return;
    }

    // Get the native window handle
    QWindow* qwindow = window->windowHandle();
    if (qwindow) {
        m_winId = reinterpret_cast<HWND>(qwindow->winId());
        initializeTaskbar();
    }
}

void WinTaskbarProgress::initializeTaskbar()
{
    if (m_initialized || !m_winId) {
        return;
    }

    // Create ITaskbarList3 instance
    HRESULT hr = CoCreateInstance(
        CLSID_TaskbarList,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITaskbarList3,
        reinterpret_cast<void**>(&m_taskbarList)
    );

    if (SUCCEEDED(hr) && m_taskbarList) {
        hr = m_taskbarList->HrInit();
        if (SUCCEEDED(hr)) {
            m_initialized = true;
        } else {
            m_taskbarList->Release();
            m_taskbarList = nullptr;
        }
    }
}

void WinTaskbarProgress::releaseTaskbar()
{
    if (m_taskbarList) {
        // Clear progress state before releasing
        if (m_winId) {
            m_taskbarList->SetProgressState(m_winId, TBPF_NOPROGRESS);
        }
        m_taskbarList->Release();
        m_taskbarList = nullptr;
    }
    m_initialized = false;
}

void WinTaskbarProgress::setValue(int value)
{
    if (!m_initialized || !m_taskbarList || !m_winId) {
        return;
    }

    // Clamp value to 0-100
    if (value < 0) value = 0;
    if (value > 100) value = 100;

    // Set progress value (current, maximum)
    m_taskbarList->SetProgressValue(m_winId, value, 100);
}

void WinTaskbarProgress::setVisible(bool visible)
{
    if (!m_initialized || !m_taskbarList || !m_winId) {
        return;
    }

    if (visible) {
        // Show normal progress
        m_taskbarList->SetProgressState(m_winId, TBPF_NORMAL);
    } else {
        // Hide progress
        m_taskbarList->SetProgressState(m_winId, TBPF_NOPROGRESS);
    }
}

void WinTaskbarProgress::reset()
{
    setVisible(false);
}

#endif // Q_OS_WIN
