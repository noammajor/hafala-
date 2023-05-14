#define _XOPEN_SOURCE 700

#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include "signals.h"
#include <csignal>
#include "Commands.h"

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    struct sigaction sigact;
    sigact.sa_handler = &alarmHandler;
    sigact.sa_flags = SA_RESTART;
    if (sigaction(SIGALRM , &sigact, 0)==-1) {
        perror("smash error: failed to set timeout handler");
    }

    SmallShell& smash = SmallShell::getInstance();
    while(true)
    {
        std::cout << smash.get_name();
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}