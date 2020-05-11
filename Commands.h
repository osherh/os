#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <list>
#include <string>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

class SmallShell;

class Command 
{
 public:
  char* cmd_line;
  std::string oldpath = "0";
  char* token;
  int special_command_num=-1;
  char* fname;
  bool redirection_flag = false;
  bool pipe_flag= false;
  Command(char* cmd_line);
  virtual ~Command();
  virtual void execute(SmallShell* smash) = 0;
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(char* cmd_line);
  virtual ~BuiltInCommand() {}
  void execute(SmallShell* smash) override;
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute(SmallShell* smash) override;
};

class PipeCommand : public Command 
{
 public:
  PipeCommand(char* cmd_line);
  virtual ~PipeCommand() {}
  void execute(SmallShell* smash) override;
};

class RedirectionCommand : public Command 
{
 public:
  explicit RedirectionCommand(char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute(SmallShell* smash) override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute(SmallShell* smash) override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute(SmallShell* smash) override;
};

class BgCommand : public BuiltInCommand {
 public:
  BgCommand(char* cmd_line);
  virtual ~BgCommand() {}
  void execute(SmallShell* smash) override;
};

class FgCommand : public BuiltInCommand {
 public:
  FgCommand(char* cmd_line);
  virtual ~FgCommand() {}
  void execute(SmallShell* smash) override;
};

class JobsList;

class QuitCommand : public BuiltInCommand 
{
  public:
    QuitCommand(char* cmd_line);
    virtual ~QuitCommand() {}
    void execute(SmallShell* smash) override;
};

class JobsList 
{
  class JobEntry
  {
    public:
      pid_t pid;
      int job_id;
      bool is_stopped;
      char* command;
      time_t inserted_time;
      
      JobEntry(pid_t pid, int job_id, bool is_stopped, char* command, time_t inserted_time);
      ~JobEntry();
  };
  std::list<JobEntry*>* jobs_list;  

  friend class KillCommand;
  friend class FgCommand;
  friend class BgCommand;
  friend class TimeoutCommand;

  public:
  JobsList();
  ~JobsList();
  void addJob(Command* cmd, bool isStopped = false);
  void addStoppedJob(pid_t pid, char* cmd);
  void printJobsList();
  void killAllJobs(SmallShell* smash);
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  bool isEmpty();
  bool stopped_joblist_is_empty();
};

class JobsCommand : public BuiltInCommand {
 public:
  JobsCommand(char* cmd_line);
  virtual ~JobsCommand() {}
  void execute(SmallShell* smash) override;
};

class KillCommand : public BuiltInCommand {
 public:
  KillCommand(char* cmd_line);
  virtual ~KillCommand() {}
  void execute(SmallShell* smash) override;
};

// TODO: should it really inhirit from BuiltInCommand ?
class CopyCommand : public BuiltInCommand {
 public:
  CopyCommand(char* cmd_line);
  virtual ~CopyCommand() {}
  void execute(SmallShell* smash) override;
};

class ChpromptCommand : public BuiltInCommand {
 public:
  ChpromptCommand(char* cmd_line);
  virtual ~ChpromptCommand() {}
  void execute(SmallShell* smash) override;
};

class CdCommand : public BuiltInCommand {
 public:
  CdCommand(char* cmd_line);
  virtual ~CdCommand() {}
  void execute(SmallShell* smash) override;
};

class TimeoutEntry
{
  public:
    const char* full_command;
    pid_t pid;
    time_t timestamp;
    int duration; //in seconds
};

class TimeoutCommand : public Command //TODO: check it
{
  public:
    TimeoutCommand(char* cmd_line);
    virtual ~TimeoutCommand() {}
    void execute(SmallShell* smash) override;
};

class SmallShell 
{ 
 private:
  SmallShell();
 
 public:
  bool alarm_is_set;

  Command *CreateCommand(char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(char* cmd_line);  
  void sendSignal(pid_t pid, int signal);

  //timeout
  void SetPidToTimeoutEntry(pid_t pid);
  
  //process
  bool process_status_is_done(pid_t pid);
  bool cmdIsExternal(const char* command);

  void syscallFailedMsg(std::string syscall_name);
};

extern std::string smash_msg;
extern pid_t smash_pid;
extern pid_t fg_pid;
extern std::string fg_command;
extern JobsList* jobs;
extern std::list<TimeoutEntry*>* timeouts;

#endif //SMASH_COMMAND_H_
