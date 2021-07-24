#include "webserver.h"

CWebServer::CWebServer() {

}


void CWebServer::InitSockets() {

    if (gArgs.GetArg("-nftnode", "") != "true") {
        //todo - log error - web server requires nft node
        return;
    }


    int port = atoi(gArgs.GetArg("-webport", "").c_str());

    if (port > 0) {

    }


}
