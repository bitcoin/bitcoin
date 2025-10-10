if (gArgs.IsArgSet("-bind") && !IsTorSocketConfigured()) {
    LogPrintf("Warning: -bind was specified but lacks a dedicated Tor socket; incoming Tor connections will be counted as regular connections.\n");
}

static bool IsTorSocketConfigured() {
    for (const auto& addrBind : gArgs.GetArgs("-bind")) {
        if (addrBind.find("127.0.0.1:8334") != std::string::npos) {
            return true;
        }
    }
    return false;
}
