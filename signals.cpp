#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) 
{
	cout << "smash: got ctrl-Z" << endl;
	smash.jobs->addStoppedJob(fg_pid, fg_command);
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
	pid_t curr_cmd_pid = getLastTimeoutCommandPid(); //TODO: rename and impl
	bool process_is_done = process_status_is_done(curr_cmd_pid);
	if(curr_cmd_pid != smash.smash_pid) 		//not a BuiltIn cmd
	{
		if(!process_is_done)
		{
			if(kill(curr_cmd_pid, SIGKILL) != 0)
    		{
     			syscall_failed_msg("kill");
    		}
		}
	}
	if(!process_is_done)
	{
		cout << "smash: [" + cmd_line + "] timed out!" << endl; // cmd_line is in format: timeout <duration> <command>
	}
}