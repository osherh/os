#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

//Ctrl+Z -> shell sends SIGTSTP to the process in foreground
void ctrlZHandler(int sig_num) 
{
	// TODO: Add your implementation
}

//Ctrl+C -> shell sends SIGINT to the process in foreground
void ctrlCHandler(int sig_num)
{
	cout << "smash: got ctrl-C";
	if(fg_process_pid! = -1)
	{
		kill(fg_process_pid, sig_num);
	}
	cout << "smash: process <foreground-PID> was killed";
}

void alarmHandler(int sig_num) 
{
  // TODO: Add your implementation
}