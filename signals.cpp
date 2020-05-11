#include <iostream>
#include <sstream>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) 
{
	cout << "smash: got ctrl-Z" << endl;
	//TODO: chek unfreed memory issues
	//TODO: set a flag to do this line on commands.cpp
	jobs->addStoppedJob(fg_pid, strdup(fg_command.c_str()));
	if(fg_pid != -1)
	{
		if(kill(fg_pid, SIGSTOP) != 0)
    	{
  			perror("smash error: kill failed"); 
    	}
	}
	stringstream msg;
	msg << "smash: process " << fg_pid << " was stopped";
	cout << msg.str() << endl;
}

void ctrlCHandler(int sig_num)
{
	cout << "smash: got ctrl-C" << endl;
	if(fg_pid != -1)
	{
		if(kill(fg_pid, SIGKILL) != 0)
    	{
  			perror("smash error: kill failed"); 
    	}
	}
	stringstream msg;
	msg << "smash: process " << fg_pid << " was killed"; 
	cout << msg.str() << endl;
}

void alarmHandler(int sig_num) 
{
	cout << "smash: got an alarm" << endl;
	TimeoutEntry* alarm_entry = NULL;
  	for (std::list<TimeoutEntry*>::reverse_iterator rit = timeouts->rbegin(); rit != timeouts->rend(); ++rit)
    {
        time_t curr_time = time(NULL);
        TimeoutEntry* timeout_entry = *rit;
        if(difftime(curr_time, timeout_entry->timestamp) == timeout_entry->duration)
        {
          alarm_entry = timeout_entry;
          break;
        }
    }
	pid_t inner_cmd_pid = alarm_entry->pid;
	int status;
  	bool process_is_done = waitpid(inner_cmd_pid, &status, WNOHANG) > 0;
	if(inner_cmd_pid != smash_pid) //not a BuiltIn cmd
	{
		if(!process_is_done)
		{
			if(kill(inner_cmd_pid, SIGKILL) != 0)
    		{
     	  			perror("smash error: kill failed"); 
    		}
		}
	}
	if(!process_is_done)
	{
		stringstream msg;
		msg << "smash: [" << alarm_entry->full_command << "] timed out!"; //cmd_line is in format: timeout <duration> <command>
		cout << msg.str() << endl;
	}
	//TODO: check if removal by additional params is needed
	//cant just remove by remove_if() since there can be multiple entries with this pid
    for (std::list<TimeoutEntry*>::reverse_iterator rit = timeouts->rbegin(); rit != timeouts->rend(); ++rit)
    {
      TimeoutEntry* timeout_entry = *rit; 
      if(timeout_entry->pid == inner_cmd_pid)
      {
        timeouts->remove(timeout_entry);
        break;
      }
    }
}