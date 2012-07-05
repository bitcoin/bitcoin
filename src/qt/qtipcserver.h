#ifndef QTIPCSERVER_H
#define QTIPCSERVER_H

using namespace boost;

/** define Bitcoin-Qt message queue name */
#define IPC_MQ_NAME "BitcoinURI"

/** define Bitcoin-Qt message queue maximum message number */
#define IPC_MQ_MAX_MESSAGES 2

/** define Bitcoin-Qt message queue maximum message size */
#define IPC_MQ_MAX_MESSAGE_SIZE 255

enum ipcState {
    IPC_NOT_INITIALIZED = 0,
    IPC_INITIALIZED = 1
};

/** global state declaration */
extern ipcState globalIpcState;

void ipcThread(void* pArg);
void ipcThread2(void* pArg);
void ipcInit(bool fUseMQModeOpenOnly = true, bool fInitCalledAfterRecovery = false);

#endif // QTIPCSERVER_H
