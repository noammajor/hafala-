#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <cstring>

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

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


void JobsList::addJob(Command* cmd, bool isStopped)
{
    if (isStopped)
        Stopped.push_back(JobEntry(stopped,  getNextPID()));
    else
        Stopped.push_back(JobEntry(background,  getNextPID()));
}

void JobsList::printJobsList()
{
    JobEntry bgJob = BGround.front();
    for (JobEntry bgJob: BGround)
    {
        for (JobEntry stoppedJob: Stopped)
        {
            if (stoppedJob.getJobId() < bgJob.getJobId())
                stoppedJob.printJob();
            else
                bgJob.printJob();
        }
    }
}

void JobsList::killAllJobs()
{
    BGround.clear();
    Stopped.clear();
}

void JobsList::removeFinishedJobs()
{///////////////////
    for()
}

JobsList::JobEntry * JobsList::getJobById(int jobId)
{
    for (JobEntry job : BGround)
    {
        if (job.getJobId() == jobId)
            return &job;
        if (job.getJobId() > jobId)
            break;
    }
    for (JobEntry job : Stopped)
    {
        if (job.getJobId() == jobId)
            return &job;
        if (job.getJobId() > jobId)
            return nullptr;
    }
}

void JobsList::removeJobById(int jobId)
{
    for (int i = 0; i < BGround.size() ; i++)
    {
        if (BGround[i].getJobId() == jobId)
        {
            BGround.erase(BGround.begin() + i);
            return;
        }
    }
    for (int j = 0; j < Stopped.size() ; j++)
    {
        if (Stopped[j].getJobId() == jobId) {
            Stopped.erase(Stopped.begin() + j);
            return;
        }
    }
}

JobsList::JobEntry * JobsList::getLastJob(int* lastJobId)
{

}

JobsList::JobEntry * JobsList::getLastStoppedJob(int *jobId)
{

}
private:
// TODO: Add your data members
std::string name;
SmallShell(): name(smash);
public:
Command *CreateCommand(const char* cmd_line);
void executeCommand(const char* cmd_line);
void changename(std::string namenew);
// TODO: add extra methods as needed
};

SmallShell::SmallShell()
{
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        if(instance== nullptr)
        {
            instance=SmallShell::SmallShell();
        }// Instantiated on first use.
        return instance;
    }

// TODO: add your implementation
}
int SmallShell::listsize() const
{
    return pastCd.size();
}
void SmallShell::changename(std::string namenew)
{
    if(numofwords(namenew)<2)
    {
        this->nameprompt= "smash> " ;
        return;
    }
    else
    {
        this->nameprompt=namenew;
        return;
    }
}
std::string SmallShell::returnprevois() const
{
    return this->pastCd.back();
}
void SmallShell::addcd(std::string name)
{
    pastCd.push_back(name);
}
void SmallShell::removecd()
{
    pastCd.pop_back();
}
std::string SmallShell::get_name() const
{
    this->nameprompt;
}

SmallShell::~SmallShell()
{
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:
/*
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}

JobsList::JobEntry::JobEntry(status starting, int id)
{
    begin = time(NULL);
    currentStatus = starting;
    Job_ID = id;
}

void JobsList::JobEntry::settime(time_t time)
{
    this->begin=time;
}

time_t JobsList::JobEntry::getcurrenttime()
{
    return difftime(this->begin, time(NULL));
}

int JobsList::JobEntry::getJobId()
{
    return this->Job_ID;
}

void JobsList::JobEntry::changestatus(status curr)
{
    this->currentStatus=curr;
}

void JobsList::JobEntry::printJob()
{
    cout << "[" << Job_ID << "]"; ////////////////continue
    if (currentStatus == stopped)
        cout << " (stopped)";
}

void BuiltInCommand::ChmodCommand::execute() override
{
   std::string work;
   work=newname;
   int counter=0, count=0;
   for(int i=0;i<work.len()<i++)
    {
       if(work[i]!=" " && counter<2)
       {
           work[count];
           count++;
       }
       if(work[i]!= " " && i>0)
        {
           if(work[i-1]==" ")
               counter++;
        }
    }
   work[count]='/0';
   std::string final=work;
   if(strlen(final)>8)
    {
    changename(final.substr(8,strlen(final)));
    }
}

void BuiltInCommand::ShowPidCommand::execute() override
{
    cout<<"smash pid is "<< getpid();
}

void BuiltInCommand::GetCurrDirCommand::execute() override
{
    std::string cwd = getcwd();
    cout<<cwd;
}
int numofwords(std::string getnum) const
{
    int count=0;
    for(int i=1,i<strlen(getnum),i++)
    {
        if(getnum[i]!=" " && getnum[i-1]==" ")
            count++;
        else if(i==1 && getnum[0]!=" " && getnum[1]==" ")
            count++;
    }
}
void BuiltInCommand::ChangeDirCommand::execute() override
{
    std:string cut;
    ///must put the string in cut
    if(numofwords(cut)>2)
    {
        cout<< "smash error: cd: too many arguments";
    }


    if(cut=='-')
    {
        if(smash.listsize()>0)
        {
            if(chdir(smash.returnprevois())==-1)
                {
                //error look at later
                }
            smash.removecd();
        }
        else
            cout<<"smash error: cd: OLDPWD not set";
    }
    else if
    {
        std::string cwd = getcwd();
        smash.addcd(cwd);
        if(chdir(smash.returnprevois())==-1)
            {
                //error look at later
            }

    }



}
int numofwords(std::string getnum) const
{
    int count=0;
    for(int i=1,i<strlen(getnum),i++)
    {
        if(getnum[i]!=" " && getnum[i-1]==" ")
            count++;
        else if(i==1 && getnum[0]!=" " && getnum[1]==" ")
            count++;
    }
    return count;
}