#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

char *smash = "smash> ";

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
    //TODO: use sigaction instead
    if(signal(SIGALRM, alarmHandler)==SIG_ERR) 
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