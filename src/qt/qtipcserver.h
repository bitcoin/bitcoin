// define Bitcoin-Qt message queue name
#define BCQT_MESSAGE_QUEUE_NAME "BitcoinURL"

void ipcShutdown();
bool ipcRecover(const char* pszFilename);
void ipcThread(void* pArg);
void ipcInit(bool fUseMQModeCreateOnly = true, bool fInitCalledAfterRecovery = false);
