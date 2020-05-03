#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) 
{
	cout << "smash: got ctrl-Z" << endl;
	jobs->addStoppedJob(fg_pid, fg_command);
	if(fg_pid! = -1)
	{
		if(kill(fg_pid, SIGSTOP) != 0)
    	{
     		syscall_failed_msg("kill");
    	}
	}
	cout << "smash: process " + fg_pid + " was stopped" << endl;
}

void ctrlCHandler(int sig_num)
{
	cout << "smash: got ctrl-C" << endl;
	if(fg_pid! = -1)
	{
		if(kill(fg_pid, SIGKILL) != 0)
    	{
     		syscall_failed_msg("kill");
    	}
	}
	cout << "smash: process " + fg_pid + " was killed" << endl;
}

void alarmHandler(int sig_num) 
{
	cout << "smash: got an alarm" << endl;
	//TODO: send a SIGKILL to the command's process(unless it's the smash)
	//TODO: check what happens if i send SIGKILL to a process that is finished(zombie)
	
	//TODO: print only if process isnt done before timeout:
	cout << "smash: [" + cmd_line + "] timed out!" << endl; // cmd_line is in format: timeout <duration> <command>
}