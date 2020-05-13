#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

JobsList* jobs = new JobsList();
std::list<TimeoutEntry*>* timeouts = new std::list<TimeoutEntry*>();
pid_t smash_pid;
pid_t fg_pid = -1;
std::string fg_command = "";

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

#define MAX_ARGS_NUM 20
#define MAX_COMMAND_LENGTH 80

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
    //TODO: replace with strcpy(args[i], _trim(s.c_str()));
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

SmallShell::SmallShell() 
{
  this->alarm_is_set = false;
  smash_pid = getpid();
}

SmallShell::~SmallShell() 
{
  delete jobs;

  for (std::list<TimeoutEntry*>::iterator it = timeouts->begin(); it != timeouts->end(); ++it)
  {
    TimeoutEntry* timeout_entry = *it;
    delete timeout_entry;
  }  
  delete timeouts;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(char* cmd_line)
{
  string cmd_s = string(cmd_line);
  if(cmd_s.find(">>") != std::string::npos || cmd_s.find(">") != std::string::npos)
  {
    Command* redirection_cmd = new RedirectionCommand(cmd_line);
    if(cmd_s.find(">>") != std::string::npos)
    {
      redirection_cmd->special_command_num = 1;
    }
    else
    {
      redirection_cmd->special_command_num = 0;  
    }    
    redirection_cmd->redirection_flag = true;
    return redirection_cmd;
  }

  else if(cmd_s.find("|") != std::string::npos || cmd_s.find("|&") != std::string::npos)
  {
    Command* pipe_cmd = new PipeCommand(cmd_line);
    if(cmd_s.find("|&") != std::string::npos)
    {
      pipe_cmd->special_command_num = 3;
    }
    else
    {
      pipe_cmd->special_command_num = 2;  
    }
    pipe_cmd->pipe_flag = true;
    return pipe_cmd;
  }

  else if(cmd_s.find("timeout") == 0)
  {
    Command* timeout_cmd = new TimeoutCommand(cmd_line);
    timeout_cmd->timeout_flag = true;
    return timeout_cmd;
  }
  else if(cmd_s.find("chprompt") == 0) //1
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
    return new FgCommand(cmd_line);    
  }

 else if(cmd_s.find("bg") == 0) //8
  {
    return new BgCommand(cmd_line);    
  }

 else if(cmd_s.find("quit") == 0) //9
  {
    return new QuitCommand(cmd_line);    
  }
 else if(cmd_s.find("cp")==0)
 {
    return new CopyCommand(cmd_line);
 }
 
 else 
  {
    return new ExternalCommand(cmd_line);
  }
  return NULL;
}

Command::Command(char* cmd_line)
{
  this->cmd_line = cmd_line;
  this->redirection_flag = false;
  this->pipe_flag = false;
  this->timeout_flag = false;
  this->special_command_num =-1;
}


Command::~Command()
{

}

BuiltInCommand::BuiltInCommand(char* cmd_line) : Command(cmd_line) { }

ChpromptCommand::ChpromptCommand(char* cmd_line) : BuiltInCommand(cmd_line) { }

ShowPidCommand::ShowPidCommand(char* cmd_line) : BuiltInCommand(cmd_line) { }

GetCurrDirCommand::GetCurrDirCommand(char* cmd_line) : BuiltInCommand(cmd_line) { }

CdCommand::CdCommand(char* cmd_line) : BuiltInCommand(cmd_line) { }

JobsCommand::JobsCommand(char* cmd_line) : BuiltInCommand(cmd_line) { }

KillCommand::KillCommand(char* cmd_line) : BuiltInCommand(cmd_line) { }

FgCommand::FgCommand(char* cmd_line) : BuiltInCommand(cmd_line) { }

BgCommand::BgCommand(char* cmd_line) : BuiltInCommand(cmd_line) { }

QuitCommand::QuitCommand(char* cmd_line) : BuiltInCommand(cmd_line) { }

TimeoutCommand::TimeoutCommand(char* cmd_line) : Command(cmd_line) {}

RedirectionCommand::RedirectionCommand(char* cmd_line) : Command(cmd_line) {}

PipeCommand::PipeCommand(char* cmd_line) : Command(cmd_line) {}

CopyCommand::CopyCommand(char* cmd_line) : BuiltInCommand(cmd_line) {}

ExternalCommand::ExternalCommand(char* cmd_line) : Command(cmd_line) {}

void BuiltInCommand::execute(SmallShell* smash)
{ 
  
}

bool SmallShell::cmdIsExternal(const char* cmd_line)
{
  char** args = (char**) malloc(strlen(cmd_line) * sizeof(char*));
  _parseCommandLine(cmd_line, args);
  char* cmd_name = args[0];  
  string cmd_st = string(cmd_name);
  bool res = !(cmd_st.find("chprompt") == 0
          ||  cmd_st.find("pwd") == 0
          ||  cmd_st.find("showpid") == 0
          ||  cmd_st.find("cd") == 0
          ||  cmd_st.find("jobs") == 0
          ||  cmd_st.find("kill") == 0
          ||  cmd_st.find("fg") == 0
          ||  cmd_st.find("bg") == 0
          ||  cmd_st.find("quit") == 0
          ||  cmd_st.find("cp") == 0    
          ||  cmd_st.find("timeout") == 0);
  free(args);
  return res;
}

void SmallShell::executeCommandAux(bool need_to_wait, Command* cmd)
{
    int pid_special = fork();
    if(pid_special > 0) //father
    {
     	if(alarm_is_set)
     	{
          SetPidToTimeoutEntry(pid_special);
      		alarm_is_set = false;
    	}
    		
    	if(need_to_wait == false) //background
    	{
        jobs->addJob(pid_special, cmd->cmd_line);
    	}
    	else //foreground
    	{
      		fg_pid = pid_special;
      		fg_command = cmd->cmd_line;
      		waitpid(pid_special, NULL, 0); //pid is the son pid
      		fg_pid = -1;
      		fg_command = "";
     	}
    }
    else if (pid_special == 0) //son
   	{
     	  setpgrp();
      	cmd->execute(this);
        quick_exit(0);
   	}
   	else
   	{ 
   		syscallFailedMsg("fork");
   	}
}

void SmallShell::executeCommand(char *cmd_line)
{
// cout << cmd_line << " - part 1" << endl;
  //cout <<"cmd is "<<cmd_line << endl;
  bool need_to_wait = (_isBackgroundComamnd(cmd_line) == false);
  if(need_to_wait == false)
  {
    _removeBackgroundSign(cmd_line); 
  }
  Command* cmd = CreateCommand(cmd_line);
  if(cmd->redirection_flag == true || cmd->pipe_flag == true || cmd->timeout_flag == true || cmdIsExternal(cmd_line))
	{
    executeCommandAux(need_to_wait, cmd);  
//    cout << cmd_line << " - part 2" << endl;
  }  
  else
  {
    cmd->execute(this);
  }
}

void ChpromptCommand::execute(SmallShell* smash)
{
  char** args = (char**) malloc((MAX_ARGS_NUM) * sizeof(char*)); //20 is the max num of args 
  int args_num = _parseCommandLine(cmd_line, args);
  string end_of_prompt = "> ";
  string new_smash_msg = ""; 
  if(args_num == 1)     //chprompt only
  {
      new_smash_msg = "smash";
  }
  else
  {
      std::string str(args[1]);
      /*if(str[str.length()-1]=='\r')
      {
        str.resize(str.length()-1);
      }*/
      new_smash_msg = str; 
  }
  smash->smash_msg = new_smash_msg + end_of_prompt;
  free(args);
}

void ShowPidCommand::execute(SmallShell* smash)
{
   std::cout <<"smash pid is "<< smash_pid << std::endl;
}

void GetCurrDirCommand::execute(SmallShell* smash)
{
 char buff[MAX_COMMAND_LENGTH];
 char* curr_working_dir = getcwd(buff, MAX_COMMAND_LENGTH);
 if(curr_working_dir == NULL)
 {
    smash->syscallFailedMsg("getcwd");
 }
 std::cout << curr_working_dir << std::endl;
}

void CdCommand::execute(SmallShell* smash)
{
  char** args = (char**) malloc(MAX_ARGS_NUM * sizeof(char*));
  int args_num = _parseCommandLine(cmd_line, args);
  if(args_num > 2)
  {
    cout << "smash error: cd: too many arguments" << endl;
    return;
  }

  char buffer[MAX_COMMAND_LENGTH];
  string newpath;
  char* path = getcwd(buffer, MAX_COMMAND_LENGTH);
  if(path == NULL)
  {
    smash->syscallFailedMsg("getcwd");
  }
  newpath = args[1];
  if(newpath.find("-") == 0) //go to old path
  {
     if(smash->oldpath.find("0") == 0)
     {
        cout << "smash error: cd: OLDPWD not set"<<endl; 
     }
     else 
     {
        int number_check = chdir(smash->oldpath.c_str());
        if(number_check == -1)
        {
          smash->syscallFailedMsg("chdir");
        }
        std::string str1(path);
        smash->oldpath = str1;
    }
  }
  else if(chdir(newpath.c_str()) != 0)
  { 
   perror("smash error: chdir failed");
  }
  else
  {
    std::string str2(path);
    smash->oldpath = str2;
  }
  free(args);
}

JobsList::JobsList()
{
  jobs_list = new std::list<JobEntry*>();
}

JobsList::~JobsList()
{
  for (std::list<JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); ++it)
  {
    JobEntry* job_entry = *it;
    delete job_entry;
  }
  delete jobs_list;
}

JobsList::JobEntry* JobsList::getLastJob(int* lastJobId)
{
  std::list<JobEntry*>::reverse_iterator last_it = jobs_list->rbegin();
  JobEntry* last_job = *last_it;
  *lastJobId = last_job->job_id;
  return *last_it;
}

JobsList::JobEntry* JobsList::getLastStoppedJob(int *jobId)
{
  for (std::list<JobEntry*>::reverse_iterator rit = jobs_list->rbegin(); rit != jobs_list->rend(); ++rit)
  {
    JobEntry* curr_job = *rit;
    if(curr_job->is_stopped)
    {
      *jobId = curr_job->job_id;
      return curr_job;
    }
  }
  return NULL;
}

JobsList::JobEntry* JobsList::getJobById(int jobId)
{
  for (std::list<JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); ++it)
  {
    JobEntry* job_entry = *it;
    if(job_entry->job_id == jobId)
    {
      return job_entry;
    }
  }
  return NULL;
}

void SmallShell::syscallFailedMsg(std::string syscall_name)
{
  stringstream msg;
  msg << "smash error: " << syscall_name << " failed"; 
  perror(msg.str().c_str());
}

void SmallShell::sendSignal(pid_t pid, int signal)
{
    if(kill(pid, signal) != 0)
    {
      syscallFailedMsg("kill");
    }
}

void JobsList::killAllJobs(SmallShell* smash)
{
  int size = jobs_list->size();
  stringstream kill_msg;
  kill_msg << "smash: sending SIGKILL signal to " << size << " jobs:" << endl;
  cout << kill_msg.str();
  for (std::list<JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); ++it)
  {
    JobEntry* job_entry = *it;

    stringstream job_msg;
    job_msg << job_entry->pid << ": " << job_entry->command << endl;
    cout << job_msg.str();
  }
  for (std::list<JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); ++it)
  {
    JobEntry* job_entry = *it;
    smash->sendSignal(job_entry->pid, SIGKILL);
  }
  cout << "Linux-shell:";
}

JobsList::JobEntry::JobEntry(pid_t pid, int job_id, bool is_stopped, char* command_name, time_t inserted_time)
{
  this->pid = pid;
  this->job_id = job_id;
  this->is_stopped = is_stopped;
  
  command = (char*)malloc(strlen(command_name) + 1);
  strcpy(command, command_name);

  this->inserted_time = inserted_time;
}

JobsList::JobEntry::~JobEntry()
{
  free(command);
}

void JobsList::addJob(pid_t pid, char* command, bool is_stopped)
{
  /*
    cout << "addJob() start" << endl;
    if(jobs->isEmpty())
    {
        cout << "jobs list is empty" << endl;  
    }
    else
    {
      jobs->printJobsList();
    }
    */ 
      jobs->removeFinishedJobs();
      int job_id = 1;
      if(!jobs_list->empty()) 
      {
        int lastJobId = 0;
        jobs->getLastJob(&lastJobId);
        job_id = lastJobId + 1;
      }
      //cout << "addJob() cmd is " << cmd->cmd_line << " , pid = " << pid << endl;
//    cout << "job command is " << cmd->cmd_line << endl;
      JobEntry* job_entry = new JobEntry(pid, job_id, is_stopped, command, time(NULL));
/* 
      int i = 1;
      for (std::list<JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); ++it)
      {
          JobEntry* entry = *it;
          cout << "job number " << i++ << " command on list is " << entry->command << endl; 
      }
*/
      jobs_list->push_back(job_entry);
/*
      int i = 1;
      for (std::list<JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); ++it)
      {
          JobEntry* entry = *it;
          cout << "job number " << i++ << " command on list is " << entry->command << endl; 
      }
*/

   /*   
    cout << "addJob() end" << endl;  
    if(jobs->isEmpty())
    {
        cout << "jobs list is empty" << endl;  
    }
    else
    {
      jobs->printJobsList();
    }*/
}

void JobsList::addStoppedJob(pid_t pid, char* command)
{
      jobs->removeFinishedJobs();
      int job_id = 1;
      if(!jobs_list->empty())
      {
        int lastJobId = 0;
        jobs->getLastJob(&lastJobId);
        job_id = lastJobId + 1;
      }
      JobEntry* job_entry = new JobEntry(pid, job_id, true, command, time(NULL));
      jobs_list->push_back(job_entry);
}

void JobsList::removeJobById(int job_id_to_remove)
{
    jobs_list->remove_if([job_id_to_remove](JobEntry* job_entry){return job_entry->job_id == job_id_to_remove;});
/*
  for (std::list<JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); ++it)
  {
    JobEntry* job_entry = *it;
    if(job_entry->job_id == job_id)
    {
      jobs_list->remove(job_entry);
    }
  }*/
}

void JobsList::removeFinishedJobs()
{
  list<bool>* remove_indicators = new list<bool>();
  for (std::list<JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); ++it)
	{
    JobEntry* job_entry = *it;
    int status;
    bool process_status_is_done = waitpid(job_entry->pid, &status, WNOHANG) > 0;
    remove_indicators->push_back(process_status_is_done); 
	}

  int job_id_to_remove = 1; 
  for (std::list<bool>::iterator it = remove_indicators->begin(); it != remove_indicators->end(); ++it)
  {
    bool remove_job_entry = *it;
    if(remove_job_entry)
    {
        jobs->removeJobById(job_id_to_remove);
    }
    ++job_id_to_remove;
  }

  free(remove_indicators);
}

void JobsList::printJobsList()
{
  /*
    cout << "we are on print jobs" << endl;
    cout << "size before remove is " << jobs_list->size() << endl;
    if(!jobs->isEmpty())
    {
      cout << "1st job is " << (*(jobs_list->begin()))->command << endl;
    }
  */
	  jobs->removeFinishedJobs();
	  //cout << "size after remove is " << jobs_list->size() << endl;

/*
  int i = 1;
  for (std::list<JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); ++it)
	{
    JobEntry* job_entry = *it;
    cout << "job element " << i++ << " is " << job_entry->command << endl;
  }
*/

  for (std::list<JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); ++it)
  {
    JobEntry* job_entry = *it;
    //cout << "job command is: " << job_entry->command << endl;
		time_t curr_time = time(NULL);
		double time_elapsed = difftime(curr_time, job_entry->inserted_time); 
		//cout << "time passed is " << time_elapsed << endl;
    stringstream job_stream ;
    	job_stream << "[" << job_entry->job_id << "] " << job_entry->command << " : " << job_entry->pid << " " << time_elapsed << " secs";
	  	if(job_entry->is_stopped)
    	{
      		job_stream << " (stopped)";
    	}
    	cout << job_stream.str() << endl; 
  }
}

bool JobsList::isEmpty()
{
  return jobs_list->empty();
}

bool JobsList::stopped_joblist_is_empty()
{
 int count_stopped_job = 0;
 for (std::list<JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); ++it)
  {
    JobEntry* job_entry = *it;
    if(job_entry->is_stopped == true)
    {
      count_stopped_job++;
      break;
    }
  }
  return count_stopped_job == 0;
}

void JobsCommand::execute(SmallShell* smash)
{
  jobs->printJobsList();
}

void KillCommand::execute(SmallShell* smash)
{
  char** args = (char**) malloc((MAX_ARGS_NUM) * sizeof(char*));
  int args_num = _parseCommandLine(cmd_line, args);

  int sig_num = 0;

  if(args_num != 2)
  {
        perror("smash error: kill: invalid arguments");
  }
  else
  {
        //signal
        char* signal = (char*)malloc(strlen(args[1])+1); // "-" followed by signal_num
        strcpy(signal, args[1]);
        const string signal_str(signal);
        if(signal_str.find_first_of("-") != 0)
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
        free(signal);

        //job id
        int job_id = atoi(args[2]);
        JobsList::JobEntry* job_found = jobs->getJobById(job_id);
        if(job_found == NULL)
        {
          stringstream job_stream;
          job_stream << "smash error: kill: job-id " << job_id << " does not exist";
          perror(job_stream.str().c_str());
        }
        smash->sendSignal(job_found->pid, sig_num);
        stringstream kill_stream ;
        kill_stream << "signal " << sig_num << " was sent to pid " << job_found->pid;
        cout << kill_stream.str() << endl; 
  }
  free(args);
}
  

void ExternalCommand::execute(SmallShell* smash)
{ 
     std::string str(cmd_line);
          if(str[str.length()-1]=='\r')
          {
          str.resize(str.length()-1);
          }
          char cmd_l[str.length()+1];
          strcpy(cmd_l,str.c_str());
    char* args[] = { strdup("bash"), strdup("-c"), cmd_l, NULL};
    execv("/bin/bash", args);   
}

void QuitCommand::execute(SmallShell* smash)
{
  char** args = (char**) malloc((2 + MAX_ARGS_NUM) * sizeof(char*)); //20 is the max num of args + 2 for timeout and duration 
  int args_num = _parseCommandLine(cmd_line, args);
  if(args_num == 0) //no args given, just quit cmd
  {
    free(args);
    exit(1);
  }
  else
  {
    if(strcmp(args[1], "kill") == 0)
    {
      jobs->killAllJobs(smash);
      free(args);
      exit(1);
    }
    free(args);
  }
}

void FgCommand::execute(SmallShell* smash)
{
  char** args = (char**) malloc((MAX_ARGS_NUM) * sizeof(char*));
  int args_num = _parseCommandLine(cmd_line, args);

  int job_number;
  JobsList::JobEntry* wanted_job;

 if (args_num > 2)
 {
      perror("smash error: fg: invalid arguments");
 }
 else if (args_num == 1) //job_id isnt given
 {
   if (jobs->isEmpty())
   {
      perror("smash error: fg: jobs list is empty");
   }
   else
   {
      // run the max job_id process on forground 
      int last_job_id;
      wanted_job = jobs->getLastJob(&last_job_id);
      fg_pid = wanted_job->pid;
      fg_command = cmd_line;
      smash->sendSignal(wanted_job->pid, SIGCONT);
      stringstream job_stream;
      job_stream << wanted_job->command << " : " << wanted_job->pid << " " << endl;
      cout << job_stream.str();
      waitpid(wanted_job->pid, NULL, 0);
      fg_pid = -1;
      fg_command = "";
      jobs->removeJobById(wanted_job->job_id);
      return;
   }
 }
 else   //job_id is given
 {
    char* check_number = (char*)malloc(strlen(args[1])+1);
    strcpy(check_number, args[1]);
    job_number = atoi(check_number);
    free(check_number);
    if (job_number == 0 )
    {
     perror("smash error: fg: invalid arguments");
    }
    else
    {
      wanted_job = jobs->getJobById(job_number);
      if (wanted_job == NULL)
      {
        stringstream job_stream;
        job_stream << "smash error: fg: job-id " << job_number << " does not exist"; 
        perror(job_stream.str().c_str());
      }
      else
      {
       fg_pid = wanted_job->pid;
       fg_command = cmd_line;
       smash->sendSignal(wanted_job->pid, SIGCONT);
       stringstream job_stream;
       job_stream << wanted_job->command << " : " << wanted_job->pid << " " << endl;
       cout << job_stream.str();
       waitpid(wanted_job->pid, NULL, 0);
       fg_pid = -1;
       fg_command = "";
       jobs->removeJobById(wanted_job->job_id);
      }
    }
  }
  free(args);
}

void BgCommand::execute(SmallShell* smash)
{ 
  char** args = (char**) malloc((MAX_ARGS_NUM) * sizeof(char*));
  int args_num = _parseCommandLine(cmd_line, args);

  int job_num;
  JobsList::JobEntry* stopped_job;
  
  if (args_num > 3)
  {
      perror("smash error: bg: invalid arguments");
  }
  else if(args_num == 1)
  {
    if (jobs->stopped_joblist_is_empty())
    {
      perror("smash error: bg: there is no stopped jobs to resume");
    }
    else
    {
      int last_stopped_job_id;
      stopped_job = jobs->getLastStoppedJob(&last_stopped_job_id);
      smash->sendSignal(stopped_job->pid, SIGCONT);
      stringstream job_stream;
      job_stream << stopped_job->command << " : " << stopped_job->pid << " " << endl;
      cout << job_stream.str();
      stopped_job->is_stopped = false;
      return;
    }
  }
  else
  {
    char* check_num = (char*)malloc(strlen(args[1])+1);
    strcpy(check_num, args[1]);
    job_num = atoi(check_num);
    free(check_num);

    if (job_num == 0 )
    {
     perror("smash error: bg: invalid arguments");
    }
    else
    {
      stopped_job = jobs->getJobById(job_num);
      if (stopped_job == NULL)
      {
        stringstream not_exist_msg;
        not_exist_msg << "smash error: bg: job-id " << job_num << " does not exist";
        perror(not_exist_msg.str().c_str());
      }
      else if(stopped_job->is_stopped == false )
      {
        stringstream back_msg;
        back_msg<< "smash error: bg: job-id " << job_num << " is already running in the background";
        perror(back_msg.str().c_str());
      }
      else
      {
        smash->sendSignal(stopped_job->pid, SIGCONT);
        stringstream job_stream;
        job_stream<< stopped_job->command << " : " << stopped_job->pid << " " << endl;
        cout << job_stream.str();
        stopped_job->is_stopped = false;
      }
    }
  }
  free(args);
}

void TimeoutCommand::execute(SmallShell* smash)
{
	char** args = (char**) malloc((2 + MAX_ARGS_NUM) * sizeof(char*)); //20 is the max num of args + 2 for timeout and duration 
	int args_num = _parseCommandLine(cmd_line, args);
	if(args_num < 3)
	{
        perror("smash error: timeout: invalid arguments");		
	}
    char* duration = args[1];
    int duration_num = 0;
    char* duration_str = (char*)malloc(strlen(duration)+1);
    strcpy(duration_str, duration);
    duration_num = atoi(duration_str);
    free(duration_str);
    if (duration_num <= 0)
    {
    	perror("smash error: timeout: invalid arguments");
    }
    char* command = (char*)malloc(strlen(cmd_line) + 1);
    int i = 2;
    strcpy(command, args[2]);
    while(i != args_num - 1)
    {
    	strcat(command, args[++i]);
    	if(i != args_num - 1) 
    	{
    		strcat(command , " ");
    	}
    }
    alarm(duration_num);   	//sends SIGALRM signal to the calling process in duration seconds
    smash->alarm_is_set = true;
    TimeoutEntry* timeout_entry = new TimeoutEntry();
    timeout_entry->timestamp = time(NULL);
    timeout_entry->duration = duration_num;
    timeouts->push_back(timeout_entry);
    smash->executeCommand(command);
    free(args);
}

void SmallShell::SetPidToTimeoutEntry(pid_t pid)
{
    TimeoutEntry* timeout_entry= timeouts->back();
    timeout_entry->pid = pid;
    timeouts->push_back(timeout_entry);
}

//TODO: check out q152 cerr

void RedirectionCommand::execute(SmallShell* smash)
{
  char** args = (char**) malloc((MAX_ARGS_NUM) * sizeof(char*));
  int args_num = _parseCommandLine(cmd_line, args);  
   
  std::string redirection_sign = ">>";
  if(special_command_num == 0)
  {
    redirection_sign = ">";
  }

  int index =0;
   while(args[index] != redirection_sign )
   index++;

    char* fname = (char*) malloc(strlen(cmd_line) + 1); 
    char* cmd_section = (char*) malloc(strlen(cmd_line) + 1);

    int i=0;
    strcpy(cmd_section,args[i++]); 
    strcat(cmd_section , " ");
    while(i < index)
    {
        strcat(cmd_section, args[i++]);
        if(i < index) 
        {
          strcat(cmd_section, " ");
        }   
    }

   i = index + 1;
     strcpy(fname,args[i++]);
     strcat(fname , " ");
      while(i < args_num)
    {
        strcat(fname, args[i++]);
        if(i < args_num) 
        {
          strcat(fname , " ");
        }   
    }

  if(this->special_command_num == 0)
  {
    int newstdout = open(fname, O_WRONLY | O_CREAT | O_TRUNC,S_IRWXU);
    dup2(newstdout, 1);
    close(newstdout);
  }
  else if(this->special_command_num == 1)
  {
    int newstdout = open(fname, O_WRONLY | O_CREAT | O_APPEND,S_IRWXU);
    dup2(newstdout, 1);
    close(newstdout);
  }
  if(smash->cmdIsExternal(cmd_section))
      {
       Command* cmd1 = new ExternalCommand(cmd_section);
       cmd1->execute(smash);
      }
      else{
      smash->executeCommand(cmd_section);
      }
  free(cmd_section);
  free(fname);
  free(args);
}


void PipeCommand::execute(SmallShell* smash)
{
  int new_fd;
  char* cmd_section1;
  char* cmd_section2;
  
  char** args = (char**) malloc((MAX_ARGS_NUM) * sizeof(char*));
  int args_num = _parseCommandLine(cmd_line, args);
  

  cmd_section1 = (char*)malloc(strlen(cmd_line)+1);
  cmd_section2 = (char*)malloc(strlen(cmd_line)+1);
  //concat all args before |
  
  std::string pipe_sign = "|&";
  if(special_command_num == 2)
  {
    pipe_sign = "|";
  }
   int index =0;
   while(args[index] != pipe_sign )
   index++;
    

    int i=0;
    strcpy(cmd_section1,args[i++]); 
    strcat(cmd_section1 , " ");
    while(i < index)
    {
        strcat(cmd_section1, args[i++]);
        if(i < index) 
        {
          strcat(cmd_section1 , " ");
        }   
    }

    //after pipe sign
    i = index + 1;
     strcpy(cmd_section2,args[i++]);
     strcat(cmd_section2 , " ");
      while(i < args_num)
    {
        strcat(cmd_section2, args[i++]);
        if(i < args_num) 
        {
          strcat(cmd_section2 , " ");
        }   
    }
  

  if(this->special_command_num == 2)
  {
      new_fd = 1;
  }
  if(this->special_command_num == 3)
  {
      new_fd = 2;
  }
  int fd[2];
  pipe(fd);
  int pid1=fork();
  if (pid1 == 0)
  {
    dup2(fd[1],new_fd);
    close(fd[0]);
    close(fd[1]);
    if(smash->cmdIsExternal(cmd_section1))
      {
       Command* cmd1 = new ExternalCommand(cmd_section1);
       cmd1->execute(smash);
      }
      else{
      smash->executeCommand(cmd_section1);
      }
    free(cmd_section1);
    quick_exit(0);
  }
  else
  {
    int pid2=fork();
    if (pid2 == 0)
    {
      dup2(fd[0],0);
      close(fd[0]);
      close(fd[1]);  
      if(smash->cmdIsExternal(cmd_section2))
      {
       Command* cmd2 = new ExternalCommand(cmd_section2);
       cmd2->execute(smash);
      }
      else{
      smash->executeCommand(cmd_section2);
      }
      free(cmd_section2); 
      quick_exit(0);
    }
    else
    {
      close(fd[0]);
      close(fd[1]);
      waitpid(pid1,NULL,0);
      waitpid(pid2,NULL,0);
    }
  }
  free(args);
}

void CopyCommand::execute(SmallShell* smash)
{
    char** args = (char**) malloc((MAX_ARGS_NUM) * sizeof(char*));
    _parseCommandLine(cmd_line, args);
  
    char* file1 = (char*)malloc(strlen(args[1])+1);
    strcpy(file1, args[1]);

    char* file2 = (char*)malloc(strlen(args[2])+1);
    strcpy(file2, args[2]);


    free(args);

    bool need_to_wait = _isBackgroundComamnd(cmd_line) == false;

    int pid = fork();
    if(pid > 0) //father
    {
      if(need_to_wait == false)
      {
          jobs->addJob(pid, cmd_line);
      }
      else //foreground
      {
          fg_pid = pid;
          fg_command = cmd_line;
          waitpid(pid, NULL, 0); //pid is the son pid
          fg_pid = -1;
          fg_command = "";
      }
    }
    else if (pid == 0) //son
    {
        setpgrp();
        int file1_descriptor = open(file1, O_RDONLY | O_CREAT,S_IRWXU);
        if(file1_descriptor == -1)
        {
            perror("the file cant be opened");
        }
        int file2_descriptor = open(file2, O_WRONLY | O_CREAT | O_TRUNC,S_IRWXU);
        if(file2_descriptor == -1)
        {
            perror("the file cant be opened");
        }
        char* content[100];
        size_t size;
 
        while ((size = read(file1_descriptor, content, 100)) > 0) 
        {
        write(file2_descriptor, content, size);
        }
        close(file1_descriptor);
        close(file2_descriptor);  
        free(file1);
        free(file2);
        quick_exit(0);
    }
    else
    { 
        perror("can't execute the command");
    }
}
