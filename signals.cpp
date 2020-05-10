#include <iostream>
#include <sstream>
#include <string.h>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

//SmallShell& smash = SmallShell::getInstance();

void ctrlZHandler(int sig_num) 
{
	cout << "smash: got ctrl-Z" << endl;
	//TODO: chek unfreed memory issues
	smash->jobs->addStoppedJob(smash, smash->fg_pid, strdup(smash->fg_command.c_str()));
	if(smash->fg_pid != -1)
	{
		smash->sendSignal(smash->fg_pid, SIGSTOP);
	}
	stringstream msg;
	msg << "smash: process " << smash->fg_pid << " was stopped";
	cout << msg.str() << endl;
}

void ctrlCHandler(int sig_num)
{
	cout << "smash: got ctrl-C" << endl;
	if(smash->fg_pid != -1)
	{
		smash->sendSignal(smash->fg_pid, SIGKILL);
	}
	stringstream msg;
	msg << "smash: process " << smash->fg_pid << " was killed"; 
	cout << msg.str() << endl;
}

void alarmHandler(int sig_num) 
{
	cout << "smash: got an alarm" << endl;
	TimeoutEntry* timeout_entry = smash->getTimeoutEntry();
	pid_t inner_cmd_pid = timeout_entry->pid;
	bool process_is_done = smash->process_status_is_done(inner_cmd_pid);
	if(inner_cmd_pid != smash->smash_pid) //not a BuiltIn cmd
	{
		if(!process_is_done)
		{
			if(kill(inner_cmd_pid, SIGKILL) != 0)
    		{
     			smash->syscallFailedMsg("kill");
    		}
		}
	}
	if(!process_is_done)
	{
		stringstream msg;
		msg << "smash: [" << timeout_entry->full_command << "] timed out!"; //cmd_line is in format: timeout <duration> <command>
		cout << msg.str() << endl;
	}
	//TODO: check if removal by additional params is needed
	smash->removeTimeoutEntryByPid(inner_cmd_pid);
}