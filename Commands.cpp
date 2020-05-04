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

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() 
{
  smash_pid = getpid();
	char *smash_msg = "smash> ";
  fg_command = "";
  fg_pid = -1;
	jobs = new JobsList();
  timeouts = new list<TimeoutEntry*>();
}

SmallShell::~SmallShell() 
{
	delete jobs;
  
  for(TimeoutEntry* timeout_entry : timeouts)
  {
    delete timeout_entry;
  }
  delete timeouts;
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
  cmd->check_special_command();
  if(cmd.special_command_num == 0)
  { 
   redirection_command();
  }
  else if(cmd.special_command_num == 1)
  {
   redirection_command_append();
  }
  else if(cmd.special_command_num == 2)
  {
   pipe_command_stdout();
  }
  else if(cmd.special_command_num == 3)
  {
   pipe_command_stderr();
  }
  cmd->execute();
  cmd->restore_stdout();
  //TODO: Please note that you must fork smash process for some commands (e.g., external commands....)
}

void ChpromptCommand::execute()
{
  base.execute(); //calls BuiltInCommand::execute
  char new_smash_msg = smash.smash_msg;
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
   strcpy(new_smash_msg , token);
   count++;
   break;
   }
   if (count == 1)
  {
    new_smash_msg = "smash";
  }
  strncat(new_smash_msg, end_of_prompt, 2);
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

JobsList::JobsList()
{
  jobs_list = new list<JobEntry*>();
}

JobsList::~JobsList()
{
  for(JobEntry* job_entry : jobs_list)
  {
    delete job_entry;
  }
  delete jobs_list;
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

//TODO: add endl where needed

void JobsList::killAllJobs()
{
  size_t size = jobs_list.size();
  cout << "smash: sending SIGKILL signal to " + size + " jobs:" << endl;
  for(JobEntry* job_entry : jobs_list)
  {
    cout << job_entry->pid + ": " + job_entry->command << endl;
  }
  for(JobEntry* job_entry : jobs_list)
  {
    send_signal(job_entry->pid, SIGKILL)
  }
  cout << "Linux-shell:";
}

bool process_status_is_done(pid_t pid)
{
  int status;
  return waitpid(pid, &status, WNOHANG) > 0;
}

void JobEntry::JobEntry(pid_t pid, int job_id, bool is_stopped, char* command, time_t inserted_time)
{
  this->pid = pid;
  this->job_id = job_id;
  this->is_stopped = is_stopped;
  this->command = command;
  this->inserted_time = inserted_time;
}

void JobsList::addJob(Command* cmd, bool isStopped = false)
{
      smash.jobs->removeFinishedJobs();
      int job_id = 1;
      if(!jobs_list.empty()) 
      {
        int job_id = smash.jobs->getLastJob()->job_id + 1;
        job_id = max_job_id + 1;
      }
      pid_t pid = getpid();
      JobEntry* job_entry = new JobEntry(pid, job_id, iStopped, cmd->cmd_line, time(NULL))
      jobs_list.push_back(job_entry);
}

void JobsList::addStoppedJob(pid_t pid, char* cmd)
{
      smash.jobs->removeFinishedJobs();
      int job_id = 1;
      if(!jobs_list.empty())
      {
        int job_id = smash.jobs->getLastJob()->job_id + 1;
        job_id = max_job_id + 1;
      }
      JobEntry* job_entry = new JobEntry(pid, job_id, true, cmd, time(NULL))
      jobs_list.push_back(job_entry);        
}

//TODO: check if calls delete or dtor
void JobsList::removeJobById(int job_id)
{
	jobs_list.remove_if(entry => entry.job_id);
}

void JobsList::removeFinishedJobs()
{
	for(JobEntry* job_entry : jobs_list)
	{
		if(process_status_is_done(job_entry->pid))
		{
			smash.jobs->removeJobById(job_entry->job_id);
		}
	}
}

void JobsList::printJobsList()
{
	smash.jobs->removeFinishedJobs();
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
bool JobsList::isEmpty()
{
  return jobs_list.empty();
}

bool JobsList::stopped_joblist_is_empty()
{
  int count_stopped_job=0;
  for(JobEntry* job_entry : jobs_list)
  {
   if(job_entry->is_stopped == true)
   {
    count_job++;
    break;
   }
  }
  return count_job == 0;
}

void JobsCommand::execute()
{
  smash.jobs->printJobsList();
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
        JobEntry* job_found = smash.jobs->getJobById(job_id);
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
    //TODO: add strtok call
    base.execute();
    
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
		    char*[4] args= {"bash", "-c", cmd_line, NULL};
		    execv("/bin/bash", args);
	   }
     else
     { 
        perror("can't execute the command");
     }
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
      smash.jobs->killAllJobs();
      exit(1)
    }
  }
}

void FgCommand::execute()
{
 base.execute();
 int count = 0;
 char* check_number;
 int job_number;
 JobEntry* wanted_job;
 while(token!=NULL)
 {
  count++;
  token=strtok(NULL," ");
 }
 if (count > 2)
 {
    perror("smash error: fg: invalid arguments")
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
    wanted_job = smash.jobs->getLastJob(&last_job_id); //TODO - make sure this is the job with the highest job id
    smash.fg_pid = wanted_job->pid;
    smash.fg_command = cmd_line;
    send_signal(wanted_job->pid, SIGCONT);
    stringstream job_stream << wanted_job->command << " : " << wanted_job->pid << " " << endl;
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
     perror("smash error: fg: invalid arguments")
	  }
    else
    {
      wanted_job = smash.jobs->getJobById(job_number);
      if (wanted_job == NULL)
      {
        perror("smash error: fg: job-id"+ job_number +"does not exist");
	    }
      else
      {
       smash.fg_pid = wanted_job->pid;
       smash.fg_command = cmd_line;
       send_signal(wanted_job->pid, SIGCONT);
       stringstream job_stream << wanted_job->command << " : " << wanted_job->pid << " " << endl;
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
{ base.execute();
  int count = 0;
  char* check_num;
  int job_num;
  JobEntry* stopped_job;
  while(token!=NULL)
  {
   count++;
   token=strtok(NULL," ");
  }
  if (count > 3)
  {
    perror("smash error: bg: invalid arguments")
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
    send_signal(stopped_job->pid, SIGCONT);
    stringstream job_stream << stopped_job->command << " : " << stopped_job->pid << " " << endl;
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
     perror("smash error: bg: invalid arguments")
	  }
    else
    {
     stopped_job = smash.jobs->getJobById(job_num);
     if (stopped_job == NULL)
     {
      perror("smash error: bg: job-id " + job_num + " does not exist");
	 }
     else if(stopped_job->is_stopped == false )
     {
      perror("smash error: bg: job-id " + job_num + " is already running in the background");
     }
     else
     {
      send_signal(stopped_job->pid, SIGCONT);
      stringstream job_stream << stopped_job->command << " : " << stopped_job->pid << " " << endl;
      stopped_job->is_stopped = false;
	 }
    }
   }
  }
}

TimeoutCommand::TimeoutCommand(const char* cmd_line);
{
  this->cmd_line = cmd_line;
}

void TimeoutCommand::execute()
{
    strtok(cmd_line, " ");  //read the timeout command, and inc the pointer to point at duration
    char* duration = strtok(NULL, " ");
    char* number_check;
    if(token != NULL)
    {
        strcpy(number_check);
        duration_num = atoi(check_num);
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
    alarm(duration)   //arranges for a SIGALRM signal to be delivered to the calling process in duration seconds
    TimeoutEntry* timeout_entry = new TimeoutEntry();
    timeout_entry->timestamp = time(NULL);
    timeout_entry->duration = duration_num;
    //TODO: add smash_pid if it is BuiltIn cmd else add the external pid from fork on father
    smash.timeouts->push_back(timeout_entry);
    smash.executeCommand(cmd);
    //TODO: when to remove an entry from the timeouts list
}

//TODO: check out q152 cerr

int Command::check_special_command()
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
     this.special_command_num = 1;
     found_sign = true;
    }
    else
    {
     this.special_command_num = 0;
     found_sign = true;
	}
   }
   else if(token[0] == '|')
   {
    if(token[1] == '&')
    {
     this.special_command_num = 3;
     found_sign = true;
	}
    else
    {
     this.special_command_num = 2;
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

void Commad::redirection_command_append()
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