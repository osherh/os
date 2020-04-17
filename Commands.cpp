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
  return nullptr;
}

vector<string> Command::split(char* to_split, const char* seperator)
{
    vector<string> arr;
    char* current = strtok(to_split, seperator.c_str());
    while(current != NULL)
    {
        arr.push_back(current);
        current = strtok(NULL, seperator);
    }
    return arr;
}

void BuiltInCommand::execute()
{ 
  int length_line = strlen(cmd_line);
  char copy_cmd_line[length_line+1];
  strcpy(copy_cmd_line , cmd_line);
  char* token;
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
 std::cout << getcwd(buff,80) << std::endl;
}

void CdCommand::execute()
{
  base.execute();



}
