#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

char *smash = "smash> ";

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
    sig_action.sa_sigaction = alarmHandler
    sig_action.sa_flags = SA_RESTART
    sigemptyset(&sig_action.sa_mask);
    if(sigaction(SIGALRM, &sig_action, NULL) == -1)
    {
        perror("smash error: failed to set alarm handler");
    }

    while(true) 
    {
        std::cout << smash;
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}