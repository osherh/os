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

SmallShell& smash = SmallShell::getInstance();

const int stdoutfd(dup(fileno(stdout)));
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

SmallShell::SmallShell() 
{
  this->smash_pid = getpid();
	this->smash_msg = "smash> ";
  this->fg_command = "";
  this->fg_pid = -1;
	this->jobs = new JobsList();
  this->timeouts = new std::list<TimeoutEntry*>();
  this->alarm_is_set = false;
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

 else if(cmd_s.find("timeout") == 0)
 {
    return new TimeoutCommand(cmd_line);
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

void BuiltInCommand::execute()
{ 
  int length_line = strlen(cmd_line);
  char copy_cmd_line[length_line+1];
  strcpy(copy_cmd_line , cmd_line);
  token = strtok(copy_cmd_line," ");
}

bool SmallShell::cmdIsExternal(const char* cmd_line)
{
  //NOTE: strtok changes the source str
  //TODO: fix other strtok() calls like this
  char* cmd_line_copy = strdup(cmd_line);
  char* cmd_name = strtok(cmd_line_copy, " ");
  bool res = !(strcmp(cmd_name, "chprompt") == 0
          ||  strcmp(cmd_name, "pwd") == 0
          ||  strcmp(cmd_name, "showpid") == 0
          ||  strcmp(cmd_name, "cd") == 0
          ||  strcmp(cmd_name, "jobs") == 0
          ||  strcmp(cmd_name, "kill") == 0
          ||  strcmp(cmd_name, "fg") == 0
          ||  strcmp(cmd_name, "bg") == 0
          ||  strcmp(cmd_name, "quit") == 0
          ||  strcmp(cmd_name, "cp") == 0    
          ||  strcmp(cmd_name, "timeout") == 0);
  free(cmd_line_copy);
  return res;
}

void SmallShell::executeCommand(char *cmd_line)
{
  //TODO: else if -> if clauses or add else
  Command* cmd = CreateCommand(cmd_line);
  cmd->check_special_command();
  if(cmd->special_command_num == 0)
  { 
      cmd->redirection_command();
  }
  else if(cmd->special_command_num == 1)
  {
      cmd->redirection_command_append();
  }
  else if(cmd->special_command_num == 2)
  {
      cmd->pipe_command_stdout();
  }
  else if(cmd->special_command_num == 3)
  {
      cmd->pipe_command_stderr();
  }

  //TODO: move if necessary
  cmd->restore_stdout();

  //TODO: add flag if on pipe state

  if(smash.cmdIsExternal(cmd_line))
  {
    bool need_to_wait = _isBackgroundComamnd(cmd_line) == false;

  int pid = fork();
  if(pid > 0) //father
  {
    if(smash.alarm_is_set)
    {
      smash.AddPidToLastTimeoutEntry(pid);
      smash.alarm_is_set = false;
    }

    if(need_to_wait == false)
    {
      smash.jobs->addJob(this);
    }
    else //foreground
    {
      smash.fg_pid = pid;
      smash.fg_command = cmd_line;
      waitpid(pid, NULL, 0); //pid is the son pid
      smash.fg_pid = -1;
      smash.fg_command = "";
    }
  }
  else if (pid == 0) //son
  {
      setpgrp();
      cmd->execute();
  }
  else
  { 
      perror(syscallFailedMsg("fork"));
  }

  }//if cmd is external - end clause

     //for built in command
    cmd->execute();
}


//TODO: save on smash.smash_msg the new msg
void ChpromptCommand::execute()
{
  BuiltInCommand::execute(); //calls BuiltInCommand::execute
  string new_smash_msg = smash.smash_msg.c_str();
  string end_of_prompt = "> ";
  int count = 0;
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
          new_smash_msg = token;
          //strcpy(new_smash_msg , token);
          count++;
          break;
      }
      if (count == 1)
      {
          new_smash_msg = "smash";
      }
      new_smash_msg = new_smash_msg + end_of_prompt;
      //strncat(new_smash_msg, end_of_prompt, 2);
  }
}

void ShowPidCommand::execute()
{
   std::cout <<"smash pid is "<< smash.smash_pid << std::endl;
}

void GetCurrDirCommand::execute()
{
 char buff[80];
 char* curr_working_dir = getcwd(buff,80);
 if(curr_working_dir == NULL)
 {
    smash.syscallFailedMsg("getcwd");
 }
 std::cout << curr_working_dir << std::endl;
}

void CdCommand::execute()
{
  BuiltInCommand::execute();
 
  char* path;
  char* newpath;
  char* to_oldpath ="-"; 
  char buffer[80];
  path = getcwd(buffer,80);
  if(path == NULL)
  {
    smash.syscallFailedMsg("getcwd");
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
     if(strcmp(oldpath.c_str(),"0") == 0)
     {
        perror("smash error: cd: OLDPWD not set"); 
     }
     else 
     {
        int number_check = chdir(oldpath.c_str());
        if(number_check == -1)
        {
          smash.syscallFailedMsg("chdir");
        }
        oldpath = path;
        //strcpy(oldpath, path);
    }
  }
  else if(chdir(newpath) != 0)
  { 
    perror("there is no such existing path");
  }
  else
  {
    oldpath = path;
    //strcpy(oldpath, path);
  } 
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

void SmallShell::syscallFailedMsg(char* syscall_name)
{
  stringstream msg;
  msg << "smash error: " << syscall_name << " failed"; 
  perror(msg.str().c_str());
}

void SmallShell::sendSignal(pid_t pid, int signal)
{
    if(kill(pid, signal) != 0)
    {
      smash.syscallFailedMsg("kill");
    }
}

void JobsList::killAllJobs()
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
    smash.sendSignal(job_entry->pid, SIGKILL);
  }
  cout << "Linux-shell:";
}

bool process_status_is_done(pid_t pid)
{
  int status;
  return waitpid(pid, &status, WNOHANG) > 0;
}

JobsList::JobEntry::JobEntry(pid_t pid, int job_id, bool is_stopped, char* command, time_t inserted_time)
{
  this->pid = pid;
  this->job_id = job_id;
  this->is_stopped = is_stopped;
  this->command = command;
  this->inserted_time = inserted_time;
}

void JobsList::addJob(Command* cmd, bool is_stopped)
{
      smash.jobs->removeFinishedJobs();
      int job_id = 1;
      if(!jobs_list->empty()) 
      {
        int lastJobId = 0;
        smash.jobs->getLastJob(&lastJobId);
        int job_id = lastJobId + 1;
      }
      pid_t pid = getpid();
      JobEntry* job_entry = new JobEntry(pid, job_id, is_stopped, cmd->cmd_line, time(NULL));
      jobs_list->push_back(job_entry);
}

void JobsList::addStoppedJob(pid_t pid, char* cmd)
{
      smash.jobs->removeFinishedJobs();
      int job_id = 1;
      if(!jobs_list->empty())
      {
        int lastJobId = 0;
        smash.jobs->getLastJob(&lastJobId);
        int job_id = lastJobId + 1;
      }
      JobEntry* job_entry = new JobEntry(pid, job_id, true, cmd, time(NULL));
      jobs_list->push_back(job_entry);
}

void JobsList::removeJobById(int job_id)
{
  for (std::list<JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); ++it)
  {
    JobEntry* job_entry = *it;
    if(job_entry->job_id == job_id)
    {
      jobs_list->remove(job_entry);
    }
  }
}

void JobsList::removeFinishedJobs()
{
  for (std::list<JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); ++it)
	{
    JobEntry* job_entry = *it;
		if(process_status_is_done(job_entry->pid))
		{
			smash.jobs->removeJobById(job_entry->job_id);
		}
	}
}

void JobsList::printJobsList()
{
	smash.jobs->removeFinishedJobs();
  for (std::list<JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); ++it)
	{
    JobEntry* job_entry = *it;
		time_t curr_time = time(NULL);
		double time_elapsed = difftime(curr_time, job_entry->inserted_time); 
		stringstream job_stream ;
    job_stream << "[" << job_entry->job_id << "]" << job_entry->command << " : " << job_entry->pid << " " << time_elapsed << " secs";
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

void JobsCommand::execute()
{
  smash.jobs->printJobsList();
}

void KillCommand::execute()
{
  BuiltInCommand::execute(); //calls BuiltInCommand::execute

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
      }
      else if(count == 2) // we have both signal and job_id
      {
        int job_id = atoi(token);
        JobsList::JobEntry* job_found = smash.jobs->getJobById(job_id);
        if(job_found == NULL)
        {
          stringstream job_stream;
          job_stream << "smash error: kill: job-id " << job_id << " does not exist";
          perror(job_stream.str().c_str());
        }
        smash.sendSignal(job_found->pid, sig_num);
        stringstream kill_stream ;
        kill_stream << "signal " << sig_num << " was sent to pid " << job_found->pid;
        cout << kill_stream.str() << endl; 
        token = NULL;
      }
    } //end while
  } //end else (we got 2 or less args)
}

void ExternalCommand::execute()
{
    char* args[] = {"bash", "-c", cmd_line, NULL};
    execv("/bin/bash", args);
}

void QuitCommand::execute()
{
  BuiltInCommand::execute();
  int count = 0;
  char* token = strtok(NULL, " ");
  if(token == NULL) //no args given, just quit cmd
  {
    exit(1);
  }
  while(token != NULL)
  {
    if(strcmp(token, "kill") == 0)
    {
      smash.jobs->killAllJobs();
      exit(1);
    }
    token = strtok(NULL, " ");  
  }
}

void FgCommand::execute()
{
 BuiltInCommand::execute();
 int count = 0;
 char* check_number;
 int job_number;
 JobsList::JobEntry* wanted_job;
 while(token!=NULL)
 {
  count++;
  token=strtok(NULL," ");
 }
 if (count > 2)
 {
    perror("smash error: fg: invalid arguments");
 }
 else if (count == 1) //no args given
 {
   bool is_empty = smash.jobs->isEmpty();
   if (is_empty == true)
   {
    perror("smash error: fg: jobs list is empty");
   }
   else
   {
    int last_job_id;
    wanted_job = smash.jobs->getLastJob(&last_job_id);
    smash.fg_pid = wanted_job->pid;
    smash.fg_command = cmd_line;
    smash.sendSignal(wanted_job->pid, SIGCONT);
    stringstream job_stream;
    job_stream << wanted_job->command << " : " << wanted_job->pid << " " << endl;
    cout << job_stream.str();
    waitpid(wanted_job->pid, NULL, 0);
    smash.fg_pid = -1;
    smash.fg_command = "";
    smash.jobs->removeJobById(wanted_job->job_id);
    return;
   }
 }

 //TODO - the if else structure isnt well defined, else clause is missing

 //job_id is given
 strtok(cmd_line, " ");
 count = 0;
 while(token!=NULL)
 {
  count++;
  token=strtok(NULL," ");
  if (count == 1 & token!=NULL)
  {
    strcpy(check_number,token);
    job_number = atoi(check_number);
    if (job_number == 0 )
    {
     perror("smash error: fg: invalid arguments");
	  }
    else
    {
      wanted_job = smash.jobs->getJobById(job_number);
      if (wanted_job == NULL)
      {
        stringstream job_stream;
        job_stream << "smash error: fg: job-id " << job_number << " does not exist"; 
        perror(job_stream.str().c_str());
	    }
      else
      {
       smash.fg_pid = wanted_job->pid;
       smash.fg_command = cmd_line;
       smash.sendSignal(wanted_job->pid, SIGCONT);
       stringstream job_stream;
       job_stream << wanted_job->command << " : " << wanted_job->pid << " " << endl;
       cout << job_stream.str();
       waitpid(wanted_job->pid, NULL, 0);
       smash.fg_pid = -1;
       smash.fg_command = "";
       smash.jobs->removeJobById(wanted_job->job_id);
	    }
    }
  }
 }
}

void BgCommand::execute()
{ 
  BuiltInCommand::execute();
  int count = 0;
  char* check_num;
  int job_num;
  JobsList::JobEntry* stopped_job;
  while(token!=NULL)
  {
   count++;
   token=strtok(NULL," ");
  }
  if (count > 3)
  {
    perror("smash error: bg: invalid arguments");
  }
  else if(count==1)
  {
   bool is_empty = smash.jobs->stopped_joblist_is_empty();
   if (is_empty == true)
   {
    perror("smash error: bg: there is no stopped jobs to resume");
   }
   else
   {
    int last_stopped_job_id;
    stopped_job = smash.jobs->getLastStoppedJob(&last_stopped_job_id);
    smash.sendSignal(stopped_job->pid, SIGCONT);
    stringstream job_stream;
    job_stream << stopped_job->command << " : " << stopped_job->pid << " " << endl;
    cout << job_stream.str();
    stopped_job->is_stopped = false;
    return;
   }
  }
  strtok(cmd_line, " ");
  count = 0;
  while(token!=NULL)
  {
   count++;
   token=strtok(NULL," ");
   if (count == 1 & token!=NULL)
   {
    strcpy(check_num,token);
    job_num = atoi(check_num);
    if (job_num == 0 )
    {
     perror("smash error: bg: invalid arguments");
	  }
    else
    {
     stopped_job = smash.jobs->getJobById(job_num);
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
      smash.sendSignal(stopped_job->pid, SIGCONT);
      stringstream job_stream;
      job_stream<< stopped_job->command << " : " << stopped_job->pid << " " << endl;
      cout << job_stream.str();
      stopped_job->is_stopped = false;
	 }
    }
   }
  }
}

TimeoutCommand::TimeoutCommand(char* cmd_line) : Command(cmd_line) {}

TimeoutEntry::TimeoutEntry(){}

void TimeoutCommand::execute()
{
    strtok(cmd_line, " ");  //read the timeout command, and inc the pointer to point at duration
    char* duration = strtok(NULL, " ");
    char* duration_str;
    int duration_num = 0;
    if(token != NULL)
    {
        strcpy(duration_str, duration);
        duration_num = atoi(duration_str);
        if (duration_num <= 0)
        {
          perror("smash error: timeout: invalid arguments");
        }
    }
    char* command = strtok(NULL, "\0");
    if (command == NULL)
    {
        perror("smash error: timeout: invalid arguments");
    }
    smash.jobs->addJob(this);
    alarm(duration_num);   //arranges for a SIGALRM signal to be delivered to the calling process in duration seconds
    smash.alarm_is_set = true;
    TimeoutEntry* timeout_entry = new TimeoutEntry();
    timeout_entry->timestamp = time(NULL);
    timeout_entry->duration = duration_num;
    smash.timeouts->push_back(timeout_entry);
    smash.executeCommand(command);
}

pid_t SmallShell::getLastTimeoutInnerCommandPid()
{
    for (std::list<TimeoutEntry*>::reverse_iterator rit = timeouts->rbegin(); rit != timeouts->rend(); ++rit)
    {
        time_t curr_time = time(NULL);
        TimeoutEntry* timeout_entry = *rit;
        if(difftime(curr_time, timeout_entry->timestamp) == timeout_entry->duration)
        {
          return timeout_entry->pid;
        }
    }   
    return -1;
}

void SmallShell::removeTimeoutByPid(pid_t pid)
{
    //cant just remove by remove_if() since there can be multiple entries with this pid
    for (std::list<TimeoutEntry*>::reverse_iterator rit = timeouts->rbegin(); rit != timeouts->rend(); ++rit)
    {
      TimeoutEntry* timeout_entry = *rit; 
      if(timeout_entry->pid == pid)
      {
        timeouts->remove(timeout_entry);
        break;
      }
    }
}

void SmallShell::AddPidToLastTimeoutEntry(pid_t pid)
{
    TimeoutEntry* timeout_entry= timeouts->back();
    timeout_entry->pid = pid;
    timeouts->push_back(timeout_entry);
}

//TODO: check out q152 cerr

void Command::check_special_command()
{
  bool found_sign= false;
  int length_cmd = strlen(cmd_line);
  char copy_cmd[length_cmd+1];
  strcpy(copy_cmd , cmd_line);
  token = strtok(copy_cmd," ");
  while(token!=NULL)
  { 
    if(found_sign==true)
    {
      strcpy(fname,token); 
    }
    if(token[0] == '>')
    { 
      if(token[1] == '>')
      {
        this->special_command_num = 1;
        found_sign = true;
      }
      else
      {
        this->special_command_num = 0;
        found_sign = true;
      }
    }
    else if(token[0] == '|')
    {
      if(token[1] == '&')
      {
        this->special_command_num = 3;
        found_sign = true;
	    }
      else
      {
        this->special_command_num = 2;
        found_sign = true;
	    }
    }
    token=strtok(NULL," ");
  }
}

void Command::redirection_command()
{ 
  int newstdout = open(this->fname, O_WRONLY | O_CREAT | O_TRUNC);
  dup2(newstdout, fileno(stdout));
  close(newstdout);
}

void Command::redirection_command_append()
{
  int newstdout = open(this->fname, O_WRONLY | O_CREAT | O_APPEND);
  dup2(newstdout, fileno(stdout));
  close(newstdout);
}

void Command::restore_stdout()
{
  fflush(stdout);
  dup2(stdoutfd, fileno(stdout));
}

void CopyCommand::execute()
{
    BuiltInCommand::execute();
    char* bg_sign;
    int index = 0;
    bool need_to_wait = true;
    while(token != NULL)
    {
      strcpy(bg_sign, token);
      token = strtok(NULL, " ");
      if(token == NULL)
      {
        while(bg_sign[index] != NULL)
        {
          char sig_bg = bg_sign[index];
          index++; 
          if(bg_sign[index] == NULL)
          {
            if(sig_bg == '&')
            {
              need_to_wait = false;
            }
	        } 
        }
      }
    }
    token = strtok(cmd_line," ");
    token = strtok(NULL," ");
    char* file1;
    char* file2;
    strcpy(file1,token);
    token = strtok(NULL," ");
    strcpy(file2,token);
    int pid = fork();
    if(pid > 0) //father
    {
      if(need_to_wait == false)
      {
          smash.jobs->addJob(this);
      }
      else //foreground
      {
          smash.fg_pid = pid;
          smash.fg_command = cmd_line;
          waitpid(pid, NULL, 0); //pid is the son pid
          smash.fg_pid = -1;
          smash.fg_command = "";
      }
    }
    else if (pid == 0) //son
    {
        setpgrp();
        int check_file1 = open(file1, O_RDONLY | O_CREAT);
        if(check_file1 == -1)
        {
            perror("the file cant be opened");
        }
        int check_file2 = open(file2, O_WRONLY | O_CREAT | O_TRUNC);
        if(check_file2 == -1)
        {
            perror("the file cant be opened");
        }
        char* content;
        rewind(file1);
        rewind(file2); 
        while(feof(file1) == 0)
        {
            //TODO: replace with read() and write()
            fgets(content,100,file1);
            fputs(content,file2);
        }
        close(check_file1);
        close(check_file2);  
    }
    else
    { 
        perror("can't execute the command");
    }  
}