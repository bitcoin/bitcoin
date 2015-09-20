// Get time from random server.
int64_t NtpGetTime();

// Get time from random server and return server address.
int64_t NtpGetTime(CNetAddr& ip);

// Get time from provided server.
int64_t NtpGetTime(std::string &strHostName);
