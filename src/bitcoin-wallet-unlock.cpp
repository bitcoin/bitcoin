// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>

void xclose(int fd)
{
    if (close(fd) == -1) {
        std::ostringstream os;
        os << "close(): " << strerror(errno);
        throw std::runtime_error(os.str());
    }
}

void xdup2(int oldfd, int newfd)
{
    if (dup2(oldfd, newfd) == -1) {
        std::ostringstream os;
        os << "dup2(): " << strerror(errno);
        throw std::runtime_error(os.str());
    }
}

void xpipe(int pipefd[2])
{
    if (pipe(pipefd) == -1) {
        std::ostringstream os;
        os << "pipe(): " << strerror(errno);
        throw std::runtime_error(os.str());
    }
}

pid_t xfork()
{
    pid_t pid = fork();
    if (pid == -1) {
        std::ostringstream os;
        os << "fork(): " << strerror(errno);
        throw std::runtime_error(os.str());
    }
    return pid;
}

// Pinentry is a simple wrapper for pinentry, which communicates with its parent
// process using a simplified subset of the assuan protcol over stdin/stdout.
// The relevant aspects of the pinentry protocol are documented at
// http://info2html.sourceforge.net/cgi-bin/info2html-demo/info2html?%28pinentry%29Protocol
class Pinentry
{
public:
    Pinentry() = delete;
    Pinentry(const Pinentry& other) = delete;
    Pinentry(int read, int write)
        : read_(fdopen(read, "r")), write_(fdopen(write, "w")), line_(nullptr), line_sz_(0) {}
    ~Pinentry()
    {
        fclose(read_);
        fclose(write_);
        free(line_);
    }

    void CheckOK()
    {
        ReadLine();
        if (strncmp(line_, "OK", 2) != 0) {
            throw std::runtime_error("pinentry protocol failure");
        }
    }

    std::string ReadData()
    {
        size_t sz = ReadLine();
        if (strncmp(line_, "D ", 2) != 0) {
            throw std::runtime_error("pinentry protocol failure");
        }
        return {&line_[2], sz - 3};
    }

    void Command(const std::string& cmd)
    {
        fprintf(write_, "%s\n", cmd.c_str());
        fflush(write_);
    }

private:
    FILE* read_;
    FILE* write_;
    char* line_;
    size_t line_sz_;

    ssize_t ReadLine()
    {
        ssize_t nread = getline(&line_, &line_sz_, read_);
        if (nread < 0) {
            throw std::runtime_error("failed to read data from pinentry");
        }
        return nread;
    }
};

// GetPinentryPassphrase invokes pinentry to get the wallet passphrase.
//
// If there were extra parameters not parsed by getopt_long() they will be
// forwarded to pinentry. Consider the following invocation:
//
//   bitcoin-wallet-unlock 300 -- -D :0
//
// This would cause pinentry to be invoked as:
//
//   pinentry -D :0
//
// Ordinarily this is not needed, but this can be useful in contexts where
// pinentry has to be explicitly told where to prompt for inputs (e.g. to force
// pinentry to use a TTY, or the opposite). In most cases they will need to be
// separated using -- as in the example above to prevent getopt_long() to
// interpret these arguments as program flags.
//
// These extra arguments are forwarded directly to execvp(), so there are no
// shell expansion or escaping issues to be aware of.
static std::string GetPinentryPassphrase(
    const std::string& pinentry_program,
    bool force_tty,
    int argc,
    char** argv)
{
    int read_pipe[2];
    xpipe(read_pipe);
    int parent_read = read_pipe[0];
    int child_write = read_pipe[1];

    int write_pipe[2];
    xpipe(write_pipe);
    int child_read = write_pipe[0];
    int parent_write = write_pipe[1];

    pid_t pid = xfork();
    if (pid > 0) {
        // parent process
        xclose(child_read);
        xclose(child_write);

        Pinentry pinentry(parent_read, parent_write);
        pinentry.CheckOK();

        if (force_tty) {
            std::ostringstream os;
            os << "OPTION ttyname=" << ttyname(STDIN_FILENO);
            pinentry.Command(os.str());
            pinentry.CheckOK();

            char* envvar;
            if ((envvar = getenv("TERM"))) {
                os.str("");
                os << "OPTION ttytype=" << envvar;
                pinentry.Command(os.str());
                pinentry.CheckOK();
            }

            if ((envvar = getenv("LC_ALL"))) {
                os.str("");
                os << "OPTION lc-ctype=" << envvar;
                pinentry.Command(os.str());
                pinentry.CheckOK();
            } else if ((envvar = getenv("LANG"))) {
                os.str("");
                os << "OPTION lc-ctype=" << envvar;
                pinentry.Command(os.str());
                pinentry.CheckOK();
            }
        }

        pinentry.Command("SETDESC Enter your Bitcoin wallet passphrase.");
        pinentry.CheckOK();

        pinentry.Command("SETPROMPT Passphrase:");
        pinentry.CheckOK();

        pinentry.Command("SETTITLE Unlock Bitcoin wallet");
        pinentry.CheckOK();

        pinentry.Command("GETPIN");
        std::string passphrase = pinentry.ReadData();
        pinentry.CheckOK();
        return passphrase;
    } else {
        // child process
        xclose(parent_read);
        xclose(parent_write);
        xdup2(child_read, STDIN_FILENO);
        xdup2(child_write, STDOUT_FILENO);
        xclose(child_read);
        xclose(child_write);

        // build an arg array for execvp
        size_t argcount = argc + 2;
        char** pinentry_args = new char*[argcount];
        pinentry_args[0] = strdup(pinentry_program.c_str());
        pinentry_args[argcount - 1] = nullptr;
        for (int i = 0; i < argc; i++) {
            pinentry_args[i + 1] = argv[i];
        }
        execvp(pinentry_args[0], pinentry_args);
        perror("failed to exec pinentry");
        exit(EXIT_FAILURE);
    }
    return 0;
}

// InvokeBitcoinCLI calls bitcoin-cli with the wallet passphrase and timeout.
static void InvokeBitcoinCLI(
    const std::string& bitcoin_cli_program,
    const std::string& passphrase,
    int timeout)
{
    int read_pipe[2];
    xpipe(read_pipe);
    int parent_read = read_pipe[0];
    int child_write = read_pipe[1];

    int write_pipe[2];
    xpipe(write_pipe);
    int child_read = write_pipe[0];
    int parent_write = write_pipe[1];

    pid_t pid = xfork();
    if (pid > 0) {
        // parent process
        xclose(child_read);
        xclose(child_write);

        FILE* child_stdin = fdopen(parent_write, "w");
        fprintf(child_stdin, "%s\n%d\n", passphrase.c_str(), timeout);
        fclose(child_stdin);

        int wstatus;
        if (waitpid(pid, &wstatus, 0) == -1) {
            std::ostringstream os;
            os << "waitpid(): " << strerror(errno);
            throw std::runtime_error(os.str());
        }
        int exit_status = WEXITSTATUS(wstatus);
        if (exit_status != 0) {
            std::ostringstream os;
            os << "bitcoin-cli exited with status " << exit_status;
            throw std::runtime_error(os.str());
        }
        xclose(parent_read);
    } else {
        // child process
        xclose(parent_read);
        xclose(parent_write);
        xdup2(child_read, STDIN_FILENO);
        xdup2(child_write, STDOUT_FILENO);
        xclose(child_read);
        xclose(child_write);
        const char* cli_args[] = {
            bitcoin_cli_program.c_str(),
            "-stdin",
            "walletpassphrase",
            nullptr};
        execvp(cli_args[0], (char* const*)cli_args);
        perror("Failed to exec bitcoin-cli");
        exit(EXIT_FAILURE);
    }
}

static void PrintUsage()
{
    std::cerr << "Usage: bitcoin-wallet-unlock [options] TIMEOUT\n";
    std::cerr << "       bitcoin-wallet-unlock [options] TIMEOUT -- pinentry_arg1 pinentry_arg2...\n";
    std::cerr << "\n";
    std::cerr << "Options:\n";
    std::cerr << "  -h, --help                         Show help\n";
    std::cerr << "  -c, --bitcoin-cli PROGRAM          Set the bitcoin-cli program (default: bitcoin-cli)\n";
    std::cerr << "  -p, --pinentry-program PROGRAM     Set the pinentry program (default: pinentry)\n";
    std::cerr << "  -t, --tty                          Force pinentry into TTY mode\n";
}

int main(int argc, char** argv)
{
    bool force_tty = false;
    std::string pinentry_program = "";
    std::string bitcoin_cli_program = "bitcoin-cli";
    static const char short_opts[] = "c:hp:t";
    static struct option long_opts[] = {
        {"help", no_argument, 0, 'h'},
        {"bitcoin-cli", required_argument, 0, 'c'},
        {"pinentry-program", required_argument, 0, 'p'},
        {"tty", no_argument, 0, 't'},
        {0, 0, 0, 0}};

    for (;;) {
        int c = getopt_long(argc, argv, short_opts, long_opts, nullptr);
        if (c == -1) {
            break;
        }
        switch (c) {
        case 'h':
            PrintUsage();
            return 0;
            break;
        case 'c':
            bitcoin_cli_program = optarg;
            break;
        case 'p':
            pinentry_program = optarg;
            break;
        case 't':
            force_tty = true;
            break;
        case '?':
            // getopt_long should already have printed an error message
            break;
        default:
            std::cerr << "Unrecognized command line flag: " << optarg << "\n";
            abort();
        }
    }
    if (pinentry_program.empty()) {
        pinentry_program = force_tty ? "pinentry-curses" : "pinentry";
    }
    if (optind >= argc) {
        std::cerr << "Error: no TIMEOUT argument was supplied.\n\n";
        PrintUsage();
        return 1;
    }

    int unlocktime;
    try {
        unlocktime = std::stod(argv[optind]);
    } catch (const std::invalid_argument&) {
        std::cerr << "Error: invalid timeout value " << argv[optind] << "\n";
        return 1;
    }
    if (unlocktime <= 0) {
        std::cerr << "Error: invalid timeout value " << argv[optind] << "\n";
        return 1;
    }

    try {
        const std::string passphrase = GetPinentryPassphrase(
            pinentry_program,
            force_tty,
            argc - optind - 1,
            &argv[optind + 1]);
        InvokeBitcoinCLI(bitcoin_cli_program, passphrase, unlocktime);
    } catch (const std::exception& exc) {
        std::cerr << "Fatal exception: " << exc.what() << "\n";
    }
    return 0;
}
