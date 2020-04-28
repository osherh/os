#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() {
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line)
{
  string cmd_s = string(cmd_line);

  if(cmd_s.find("chprompt") == 0) //1
  {
    return new ChpromptCommand(cmd_line);
  }
  
  else if(cmd_s.find("pwd") == 0) //2
  {
    return new GetCurrDirCommand(cmd_line);
  }

  else if(cmd_s.find("showpid") == 0) //3
  {
    return new ShowPidCommand(cmd_line);
  }
  
  else if(cmd_s.find("cd") == 0) //4
  {
    return new CdCommand(cmd_line);    
  }

  else if(cmd_s.find("jobs") == 0) //5
  {
    return new JobsCommand(cmd_line);    
  }

  else if(cmd_s.find("kill") == 0) //6
  {
    return new KillCommand(cmd_line);    
  }

 else if(cmd_s.find("fg") == 0) //7
  {
    return new FgCommnad(cmd_line);    
  }

 else if(cmd_s.find("bg") == 0) //8
  {
    return new BgCommnad(cmd_line);    
  }

 else if(cmd_s.find("quit") == 0) //9
  {
    return new QuitCommand(cmd_line);    
  }

  else 
  {
    return new ExternalCommand(cmd_line);
  }
  return NULL;
}

void BuiltInCommand::execute()
{ 
  int length_line = strlen(cmd_line);
  char copy_cmd_line[length_line+1];
  strcpy(copy_cmd_line , cmd_line);
  token = strtok(copy_cmd_line," ");
}

void SmallShell::executeCommand(const char *cmd_line)
{
  Command* cmd = CreateCommand(cmd_line);

  cmd->execute();
  //TODO: Please note that you must fork smash process for some commands (e.g., external commands....)
}

void ChpromptCommand::execute()
{
  base.execute(); //calls BuiltInCommand::execute
  char new_smash= smash;
  char* end_of_prompt ="> ";
  int count=0;
  while (token != NULL)
  {
   if (count == 0) //pass the first word(because its chprompt)
   { 
       token = strtok(NULL, " ");
       count++;
       continue;
   }
   else if (count == 1) //the next prompt we need to print
   {
   strcpy(new_smash , token);
   count++;
   break;
   }
   if (count == 1)
  {
    new_smash = "smash";
  }
  strncat(new_smash, end_of_prompt, 2);
}

void ShowPidCommand::execute()
{
   std::cout <<"smash pid is "<< ::getpid() << std::endl;
}

void GetCurrDirCommand::execute()
{
 char buff[80];
 char* curr_working_dir = getcwd(buff,80);
 if(curr_working_dir == NULL)
 {
    syscall_failed_msg("getcwd");
 }
 std::cout << curr_working_dir << std::endl;
}

void CdCommand::execute()
{
  base.execute();
 
  char* path;
  char* newpath;
  char* to_oldpath ="-"; 
  char buffer[80];
  path = getcwd(buffer,80);
  if(path == NULL)
  {
    syscall_failed_msg("getcwd");
  }
  int count=0;

  while(token != NULL)
  {
    if (count== 0)
    {
      token = strtok(NULL, " ");
      count++;
      continue;
    }
    else if(count==1)
    {
      count ++;
      strcpy(newpath , token);
      token = strtok(NULL, " ");
    }
    else if(count == 2)
    {
      perror("smash error: cd: too many arguments");
      return;
    }
  }
  if(strcmp(newpath,to_oldpath) == 0)
  {
     if(strcmp(oldpath,"0") == 0)
     {
        perror("smash error: cd: OLDPWD not set"); 
     }
     else 
     {
        int number_check = chdir(oldpath);
        if(number_check == -1)
        {
          syscall_failed_msg("chdir");
        }
        strcpy(oldpath, path);
    }
  }
  else if(chdir(newpath != 0))
  { 
    perror("there is no such existing path");
  }
  else
  {
    strcpy(oldpath, path);
  } 
}

JobEntry* JobsList::getLastJob(int* lastJobId)
{
  list<JobEntry*>::reverse_iterator last_it = jobs_list.rbegin();
  *lastJobId = curr_job->job_id;
  return *last_it;
}

JobEntry* JobsList::getLastStoppedJob(int *jobId)
{
  for (list<JobEntry*>::reverse_iterator rit = jobs_list.rbegin(); rit != jobs_list.rend(); ++rit)
  {
    JobEntry* curr_job = *rit;
    if(curr_job->is_stopped)
    {
      *jobId = curr_job->job_id;
      return curr_job;
    }
  }
}

JobEntry* JobsList::getJobById(int jobId)
{
  for(JobEntry* job_entry : jobs_list)
  {
    if(job_entry->job_id == jobId)
    {
      return job_entry;
    }
  }
  return NULL;
}

void syscall_failed_msg(char* syscall_name)
{
  perror("smash error: " + syscall_failed_msg + " failed");
}

void send_signal(pid_t pid, int signal)
{
    if(kill(pid, signal) != 0)
    {
      syscall_failed_msg("kill");
    }
}

void JobsList::killAllJobs()
{
  for(JobEntry* job_entry : jobs_list)
  {
    send_signal(job_entry->pid, SIGKILL)
  }
}

bool process_status_is_done(pid_t pid)
{
  int status;
  return waitpid(pid, &status, WNOHANG) > 0;
}

bool less_than_by_job_id(const JobEntry* &a, const JobEntry* &b)
{
    return a->job_id < b->job_id;
}

void JobsList::addJob(Command* cmd, bool isStopped = false)
{
      removeFinishedJobs();
      
      int job_id = 1;
      if(!jobs_list.empty())     
      {
        auto max_job_id_it = max_element(jobs_list.begin(), jobs_list.end(), less_than_by_job_id);
        int max_job_id = *max_job_id_it;
        job_id = max_job_id + 1;
      }
      pid_t pid = getpid();
      bool is_done = process_status_is_done(pid);
      JobEntry* job_entry = new JobEntry(pid, job_id, iStopped, is_done, cmd->cmd_line, time(NULL))
      jobs_list.push_back(job_entry);
}

//TODO: call before executing any command
//TODO: check if calls delete or dtor
void JobsList::removeJobById(int job_id)
{
	jobs_list.remove_if(entry => entry.job_id);
}

void JobsList::removeFinishedJobs()
{
	for(JobEntry* job_entry : jobs_list)
	{
		if(job_entry->is_done)
		{
			jobs_list.removeJobById(job_entry->job_id);
		}
	}
}

void JobsList::printJobsList()
{
	removeFinishedJobs();
	for(JobEntry* job_entry : jobs_list)
	{
		time_t curr_time = time(NULL);
		double time_elapsed = difftime(curr_time, job_entry->inserted_time); 
		stringstream job_stream << "[" << job_entry->job_id << "]" << job_entry->command << " : " << job_entry->pid << " " << time_elapsed << " secs";
	  if(job_entry->is_stopped)
    {
      job_stream << " (stopped)";
    }
    cout << job_stream.c_str(); //TODO - fix 
  }
}

void KillCommand::execute()
{
  base.execute(); //calls BuiltInCommand::execute

  char* signal; // "-" followed by signal_num
  int count = 0;
  int sig_num = 0;

  //check how many args are given
  while(token != NULL)  
  { 
    token = strtok(NULL, " ");
    count++;
    continue;
  }
  
  strtok(cmd_line, " "); //reset the pointer

  if(count > 2)
  {
    perror("smash error: kill: invalid arguments");
  }
  else
  {
    count = 0;
    //handle the cases
    while(token != NULL)
    {
      if(count== 0)  //cmd
      {
        token = strtok(NULL, " ");
        if(token == NULL) //there are no args for kill command
        {
          perror("smash error: kill: invalid arguments");
        }
        count++;
        continue;
      }
      else if(count == 1) //signal
      {
        count++;      
        strcpy(signal, token);
        token = strtok(NULL, " ");
        if(token == NULL) //there is no job_id specified
        {
          perror("smash error: kill: invalid arguments");
        }
        if(0 != strcmp(signal[0],"-"))
        {
          perror("smash error: kill: invalid arguments");
        }
        else
        {
          char* signal_num;
          signal_num = strtok(signal, "-");
          sig_num = atoi(signal_num);
          if(sig_num < 1 || sig_num > 31) // if signal_num isnt a number then sig_num == 0   
          {
            perror("smash error: kill: invalid arguments");
          }
        }
      }
      else if(count == 2) // we have both signal and job_id
      {
        int job_id = atoi(token);
        JobEntry* job_found = getJobById(job_id); //TODO - instance method, fix call
        if(job_found == NULL)
        {
          perror("smash error: kill: job-id " + job_id + " does not exist");
        }
        send_signal(job_found->pid, sig_num);
        stringstream kill_stream << "signal " << sig_num << " was sent to pid " << job_found->pid;
        cout << job_kill.c_str(); //TODO - fix 
        token = NULL;
      }
    } //end while
  } //end else (we got 2 or less args)
}

void ExternalCommand::execute()
{
	fork();
	int pid = getpid(); 
	if(pid > 0) //father
	{
		waitpid(pid, NULL, WNOHANG); //pid is the son pid
	}
	else //getpid() == 0) //son
	{
		char*[] args = new char*[3] {"-c", cmd_line, NULL};
		execv("/bin/bash", args);
	}

  //TODO - wait, waitpid are blocking syscalls. Note that wait() syscall will block unless WHOHANG is given in the options
}

QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs)
{
  this.jobs = jobs;
}

void QuitCommand::execute()
{
  base.execute();
  int count = 0;
  char* token = strtok(NULL, " ");
  if(token == NULL) //no args given, just quit cmd
  {
    exit(1);
  }
  else
  {
    if(strcmp(token, "kill") == 0)
    {
      jobs->killAllJobs();
      exit(1)
    }
  }
}