#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

struct sigaction sig_action;

int main(int argc, char* argv[]) 
{
    if(signal(SIGTSTP, ctrlZHandler)==SIG_ERR) 
    {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT, ctrlCHandler)==SIG_ERR) 
    {
        perror("smash error: failed to set ctrl-C handler");
    }

    memset(&sig_action, 0, sizeof(sig_action));
    sig_action.sa_handler = alarmHandler;
    sig_action.sa_flags = SA_RESTART;
    sigemptyset(&sig_action.sa_mask);
    if(sigaction(SIGALRM, &sig_action, NULL) == -1)
    {
        perror("smash error: failed to set alarm handler");
    }

    SmallShell& smash = SmallShell::getInstance();
    while(true) 
    {
        std::cout << "smash> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        char* cmd = strdup(cmd_line.c_str());
        smash.executeCommand(cmd);
        free(cmd);
    }
    return 0;
}