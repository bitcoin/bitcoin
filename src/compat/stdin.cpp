// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compat/stdin.h>

#include <cstdio>

#ifdef WIN32
#include <windows.h>
#include <io.h>
#else
#include <termios.h>
#include <unistd.h>
#include <poll.h>
#endif

// https://stackoverflow.com/questions/1413445/reading-a-password-from-stdcin
void SetStdinEcho(bool enable)
{
    if (!StdinTerminal()) {
        return;
    }
#ifdef WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    if (!GetConsoleMode(hStdin, &mode)) {
        fputs("GetConsoleMode failed\n", stderr);
        return;
    }
    if (!enable) {
        mode &= ~ENABLE_ECHO_INPUT;
    } else {
        mode |= ENABLE_ECHO_INPUT;
    }
    if (!SetConsoleMode(hStdin, mode)) {
        fputs("SetConsoleMode failed\n", stderr);
    }
#else
    struct termios tty;
    if (tcgetattr(STDIN_FILENO, &tty) != 0) {
        fputs("tcgetattr failed\n", stderr);
        return;
    }
    if (!enable) {
        tty.c_lflag &= static_cast<decltype(tty.c_lflag)>(~ECHO);
    } else {
        tty.c_lflag |= ECHO;
    }
    if (tcsetattr(STDIN_FILENO, TCSANOW, &tty) != 0) {
        fputs("tcsetattr failed\n", stderr);
    }
#endif
}

bool StdinTerminal()
{
#ifdef WIN32
    return _isatty(_fileno(stdin));
#else
    return isatty(fileno(stdin));
#endif
}

bool StdinReady()
{
    if (!StdinTerminal()) {
        return true;
    }
#ifdef WIN32
    return false;
#else
    struct pollfd fds;
    fds.fd = STDIN_FILENO;
    fds.events = POLLIN;
    return poll(&fds, 1, 0) == 1;
#endif
}

NoechoInst::NoechoInst() { SetStdinEcho(false); }
NoechoInst::~NoechoInst() { SetStdinEcho(true); }
