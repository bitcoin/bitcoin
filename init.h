// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

void Shutdown(void* parg);
bool AppInit(int argc, char* argv[]);
bool AppInit2(int argc, char* argv[]);

#ifdef USE_POSIX_CAPABILITIES
extern "C" {
    extern int cCap_lock(void);
}
#endif

