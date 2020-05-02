#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) 
{
	cout << "smash: got ctrl-Z";
	jobs->addStoppedJob(fg_pid, fg_command);
	if(fg_pid! = -1)
	{
		if(kill(fg_pid, SIGSTOP) != 0)
    	{
     		syscall_failed_msg("kill");
    	}
	}
	cout << "smash: process " + fg_pid + " was stopped";
}

void ctrlCHandler(int sig_num)
{
	cout << "smash: got ctrl-C";
	if(fg_pid! = -1)
	{
		if(kill(fg_pid, SIGKILL) != 0)
    	{
     		syscall_failed_msg("kill");
    	}
	}
	cout << "smash: process " + fg_pid + " was killed";
}

void alarmHandler(int sig_num) 
{
	//TODO: send a SIGKILL to the command's process(unless it's the smash)

	cout << "smash: [" + cmd_line + "] timed out!"; // cmd_line is in format: timeout <duration> <command>
}