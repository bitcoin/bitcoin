void ipcShutdown();
bool ipcRecover(const char* pszFilename);
void ipcThread(void* pArg);
void ipcInit(bool fUseMQModeCreateOnly = true, bool fInitCalledAfterRecovery = false);
