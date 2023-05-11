#include "Commands.h"
#include <cstring>
#include <chrono>


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


SmallShell* SmallShell::instance = nullptr;

///////////////////////////////////////////////  General Functions   ///////////////////////////////////////////////////////////

std::string fileNameOpen(const char* cmd_line)
{
    std::string fileName = cmd_line;
    if(fileName.find(">>") < fileName.length())
    {
        return _trim(fileName.substr(fileName.find(">>")+2,fileName.length()));
    }
    else
    {
        return _trim(fileName.substr(fileName.find('>')+1,fileName.length()));
    }
}

bool redirection(const char* cmd_line, bool setTimeout)
{
    string line = cmd_line;
    bool append = false;
    if (line.find(">>") != string::npos)
        append = true;
    std::string directFile= fileNameOpen(cmd_line);
    pid_t child = fork();
    if (child < 0)
    {
        perror("smash error: fork failed");
    }
    else if (child == 0)
    {
        if(setpgrp()==-1)
        {
            perror("smash error: setpgrp failed");
        }
        int fd;
        if (append)
        {
            fd = open(directFile.c_str(), O_APPEND|O_CREAT|O_RDWR, 0666);
        }
        if (!(append))
        {
            fd = open(directFile.c_str(), O_TRUNC|O_CREAT|O_RDWR, 0666);
        }
        if (fd < 0)
        {
            perror("Cannot open file"); ///not defined in the project
        }
        if(dup2(fd, STDOUT_FILENO) == -1)
        {
            perror("smash error: dup2 failed");
        }
        if (fd > 0)
        {
            if (close(fd) == -1)
                perror("smash error: close failed");
        }
        return true;
    }
    else     // child > 0
    {
        if (setTimeout)
        {
            Command* timeoutCmd = new TimeoutCommand(cmd_line, child);
            timeoutCmd->execute();
        }
        int status;
        if (waitpid(child, &status, 0) < 0)
            perror("smash error: waitpid failed");
    }
    return false;
}

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

bool _isBackgroundComamnd(const char* cmd_line)
{
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(string cmd_line)
{
    // find last character other than spaces
    unsigned int idx = cmd_line.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == (unsigned int)string::npos)
    {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[cmd_line.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

int numOfWords(const char* cmd_line, string* argsTable)
{
    string line = _trim(cmd_line);
    int count = 1;
    string cur;
    cur += line[0];
    int index = 0;
    for(int i = 1 ; i < (int)line.length() ; i++)
    {
        if(line[i-1] != ' ' && line[i] == ' ')
        {
            count++;
            argsTable[index++] = cur;
            cur = "";
        }
        else
            cur += line[i];
    }
    argsTable[index++] = cur;
    argsTable[index] = '\0';
    return count;
}

void splitByArg(const char* line, char arg, char** result)
{
    string lineString = _trim(line);
    string arg1, arg2;
    for (int i = 0 ; i < (int)lineString.find(arg) ; i++)
    {
        arg1 += lineString[i];
    }
    arg1 += '\0';
    char* strCopy = new char[arg1.size()+1];
    std::strcpy(strCopy,arg1.c_str());
    result[0]= strCopy;
    int j = (int)lineString.find(arg) + 1;
    if (lineString[j] == '&')
        j++;
    for ( ; j < (int)lineString.length() ; j++)
        arg2 += lineString[j];
    arg2 += '\0';
    char* strCopy2 = new char[arg2.size()+1];
    std::strcpy(strCopy2,arg2.c_str());
    result[1]= strCopy2;
}

string findCommand(const char* cmd_line)
{
    string line = cmd_line;
    string command = "";
    string timeout = "timeout";
    int i = 0;
    for (; i < (int)line.length() && i < (int)timeout.length() ; i++)
    {
        if (line[i] != timeout[i])
            break;
    }
    if (i != (int)timeout.length())
        i = 0;
    else
    {
        for (;  i < (int)line.length() ; i++)
        {
            if (!strcmp(&line[i], " ") && !isdigit(line[i]))
                break;
        }
    }
    for ( ; i != (int)line.length() && isalpha(line[i]) ; i++)
        command += line[i];
    return command;
}

///////////////////////////////////////////////////////   Commands   ///////////////////////////////////////////////////////////

void Command::printComd() const
{
    cout<< cmdLine;
}

void Command::cleanup()
{
    if (getpid() != SmallShell::getInstance().getSmashPid())
    {
        if(close(1) == -1)
        {
            perror("smash error: close failed");
        }
        exit(0);
    }

}


/////////////////////////////////////////////////  Jobs   ///////////////////////////////////////////////////////////


void JobsList::addJob(const char* cmd_line, pid_t pid, bool isStopped)
{
    if (isStopped)
    {
        JobEntry* job = new JobEntry(stopped, getNextJobID(), pid, cmd_line);
        Stopped.push_back(job);
    }
    else
    {
        JobEntry* job = new JobEntry(background, getNextJobID(), pid, cmd_line);
        BGround.push_back(job);
    }
}

void JobsList::printJobsList()
{
    removeFinishedJobs();
    for (JobEntry* bgJob: BGround)
    {
        for (JobEntry* stoppedJob: Stopped)
        {
            if (stoppedJob->getJobId() < bgJob->getJobId())
                stoppedJob->printJob();
            else
                bgJob->printJob();
        }
    }
}

void JobsList::killAllJobs()
{
    for (JobEntry* job : BGround)
    {
        if (job->getPid() != SmallShell::getInstance().getSmashPid())
            kill(job->getPid(), SIGKILL);
    }
    for (JobEntry* job : Stopped)
    {
        if (job->getPid() != SmallShell::getInstance().getSmashPid())
            kill(job->getPid(), SIGKILL);
    }
    BGround.clear();
    Stopped.clear();
}

void JobsList::removeFinishedJobs()
{
    for (int i = (int)BGround.size() ; i > 0 ; i--)
    {
        int wait_res = waitpid(BGround[i-1]->getPid(), nullptr, WNOHANG);
        if(wait_res==-1)
        {
            perror("smash error: waitpid failed");
        }
        if (wait_res == BGround[i-1]->getPid())
        {
            delete BGround[i-1];
            BGround.erase(BGround.begin() + i - 1);
        }
    }
}

JobsList::JobEntry * JobsList::getJobById(int jobId)
{
    for (JobEntry* job : BGround)
    {
        if (job->getJobId() == jobId)
            return job;
        if (job->getJobId() > jobId)
            break;
    }
    for (JobEntry* job : Stopped)
    {
        if (job->getJobId() == jobId)
            return job;
        if (job->getJobId() > jobId)
            return nullptr;
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId)
{
    for (int i = 0; i < (int)BGround.size() ; i++)
    {
        if (BGround[i]->getJobId() == jobId)
        {
            BGround.erase(BGround.begin() + i);
            return;
        }
    }
    for (int j = 0; j < (int)Stopped.size() ; j++)
    {
        if (Stopped[j]->getJobId() == jobId) {
            Stopped.erase(Stopped.begin() + j);
            return;
        }
    }
}

JobsList::JobEntry * JobsList::getLastJob()
{
    int maxBG = BGround.back()->getJobId();
    int maxStopped = Stopped.back()->getJobId();
    return ( maxBG > maxStopped ? BGround.back() : Stopped.back() );
}

JobsList::JobEntry * JobsList::getLastStoppedJob()
{
    return Stopped.back();
}

int JobsList::getNextJobID ()
{
    removeFinishedJobs();
    int maxBG = 0;
    int maxStopped = 0;
    if ((int)BGround.size() > 0)
        maxBG = BGround.back()->getJobId();
    if((int)Stopped.size() > 0 )
        maxStopped = Stopped.back()->getJobId();
    return ( maxBG > maxStopped ? maxBG+1 : maxStopped+1 );
}


void JobsList::moveToFG(JobEntry* job)
{
    FGround = job;
    job->changeStatus(forground);
}


void JobsList::moveToBG(JobEntry* job)
{
    removeJobById(job->getJobId());
    job->changeStatus(background);
    BGround.push_back(job);
}

void JobsList::addToFG(JobEntry* job)
{
    FGround = job;
}

void JobsList::addToStopped(JobEntry* job)
{
    Stopped.push_back(job);
}

JobsList::JobEntry* JobsList::getFGjob() const
{
    return FGround;
}

/////////////////////////////////////////////////  JobEntry   ///////////////////////////////////////////////////////////

time_t JobsList::JobEntry::getCurrentTime()
{
    return difftime(time(NULL),this->begin);
}

int JobsList::JobEntry::getJobId()
{
    return Job_ID;
}

void JobsList::JobEntry::changeStatus(status curr)
{
    currentStatus = curr;
}

status JobsList::JobEntry::getStat()
{
    return currentStatus;
}

void JobsList::JobEntry::printJob()
{
    cout << "[" << Job_ID << "] " << cmdLine << " : " << pid <<" " << getCurrentTime()<<" secs";
    if (currentStatus == stopped)
        cout << " (stopped)";
    cout << "\n";
}

void JobsList::JobEntry::printCmd()
{
    cout << cmdLine<<endl;
}

pid_t JobsList::JobEntry::getPid() const
{
    return pid;
}

const char* JobsList::JobEntry::getCmdLine()
{
    return cmdLine;
}

void JobsList::JobEntry::FGjobID()
{
    Job_ID = -1;
}


/////////////////////////////////////////////////  SmallShell   ///////////////////////////////////////////////////////////

SmallShell::~SmallShell()
{
    jobsList->killAllJobs();
}

void SmallShell::changeName(const char* newName)
{
    namePrompt = newName;
}

pid_t SmallShell::getSmashPid() const
{
    return smashPid;
}

void SmallShell::addCD(const char* dir)
{
    curCD = dir;
}

const char* SmallShell::getCD()
{
    return curCD;
}

std::string SmallShell::get_name() const
{
    return namePrompt+"> ";
}

JobsList* SmallShell::getJobs()
{
    return jobsList;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command* SmallShell::CreateCommand(const char* cmd_line)
{
    string cmd_s = _trim(string(cmd_line));
    bool runInBack = false;
    if (_isBackgroundComamnd(cmd_line))
        runInBack = true;
    _removeBackgroundSign(cmd_s);
    bool isChild  = false;
    bool redirectionHappened = false;
    Command* cmd = nullptr;
    bool setTimeout = false;
    string firstWord = cmd_s.substr(0, cmd_s.find(' '));
    if (firstWord == "timeout")
    {
        setTimeout = true;
    }
    if(cmd_s.find('>') != string::npos || cmd_s.find(">>") != string::npos)   //redirection Builtin
    {
        redirectionHappened = true;
        isChild = redirection(cmd_line, setTimeout);
        if (isChild)
        {
            cmd = BuiltIn(cmd_line);
        }
    }
    else if (cmd_s.find('|') != string::npos)
    {
        cmd = new PipeCommand(cmd_line);
    }
    if(!cmd && redirectionHappened && isChild)      // redirection external
    {
        if (cmd_s.find('*') != string::npos || cmd_s.find('?') != string::npos)
            cmd = new ComplexCommand(cmd_line);
        else
            cmd = new SimpleCommand(cmd_line);
    }
    if(!cmd && !redirectionHappened)
    {
        cmd = BuiltIn(cmd_line);
        if(!cmd && forkExtrenal(setTimeout, runInBack, cmd_line))
        {;
            if (cmd_s.find('*') != string::npos || cmd_s.find('?') != string::npos)
                cmd = new ComplexCommand(cmd_line);
            else
            {
                cmd = new SimpleCommand(cmd_line);
            }
        }
    }
    return cmd;
}

bool SmallShell::forkExtrenal(bool setTimeout, bool runInBack, const char* cmd_line)
{
    pid_t child_pid = fork();
    if(child_pid < 0)
    {
        perror("smash error: fork failed");
    }
    else if(child_pid == 0)
    {
       if(setpgrp()==-1)
       {
           perror("smash error: setpgrp failed");
       }
        return true;
    }
    else            //  child_pid > 0
    {
        if (setTimeout)
        {
            Command* timeoutCmd = new TimeoutCommand(cmd_line, child_pid);
            timeoutCmd->execute();
        }
        if (runInBack)
        {
            SmallShell::getInstance().getJobs()->addJob(cmd_line, child_pid,false);
        }
        else
        {
            JobsList::JobEntry* job = new JobsList::JobEntry(forground, -1, child_pid, cmd_line);
            SmallShell::getInstance().getJobs()->addToFG(job);
            int status;
            if(waitpid(child_pid, &status, 0)==-1)
            {
                perror("smash error: waitpid failed");
            }
        }
    }
    return false;
}

Command* SmallShell::BuiltIn(const char* cmd_line)
{
    string firstWord = _trim(cmd_line);
    firstWord = findCommand(firstWord.c_str());
    if (firstWord == "chprompt")
        return new ChpromptCommand(cmd_line);
    else if (firstWord == "showpid")
        return new ShowPidCommand(cmd_line);
    else if (firstWord == "pwd")
        return new GetCurrDirCommand(cmd_line);
    else if (firstWord == "cd")
        return new ChangeDirCommand(cmd_line);
    else if (firstWord == "jobs")
        return new JobsCommand(cmd_line, jobsList);
    else if (firstWord == "fg")
        return new ForegroundCommand(cmd_line, jobsList);
    else if (firstWord == "bg")
        return new BackgroundCommand(cmd_line, jobsList);
    else if (firstWord == "quit")
        return new QuitCommand(cmd_line, jobsList);
    else if (firstWord == "kill")
        return new KillCommand(cmd_line, jobsList);
    else if (firstWord == "setcore")
        return new SetcoreCommand(cmd_line);
    else if (firstWord == "getfiletype")
        return new GetFileTypeCommand(cmd_line);
    else if (firstWord == "chmod")
        return new ChmodCommand(cmd_line);
    return nullptr;
}

void SmallShell::add_timeout(Timeout_obj* newTime)
{
    time_t min = newTime->alarm_time;
    for (Timeout_obj* obj : timeout)
    {
        if (obj->alarm_time < min)
            min = obj->alarm_time;
    }
    time_t setAlarm = min - time(nullptr);
    alarm(setAlarm);
    timeout.push_back(newTime);
}

std::vector<Timeout_obj*> SmallShell::getAlarmed()
{
    return timeout;
}

void SmallShell::executeCommand(const char *cmd_line) {
    char* cmd = new char[strlen(cmd_line)] ;
    strcpy(cmd, cmd_line);
    const char* cmdLine = cmd;
    Command* command = CreateCommand(cmdLine);
    if(command == nullptr)
    {
        return;
    }
    command->execute();
    command->cleanup();
}


/////////////////////////////////////////////////  Execute Commands   ///////////////////////////////////////////////////////////

void ChpromptCommand::execute()
{
    string args[22];
    int argsCnt = numOfWords(cmdLine, args);
    if (argsCnt == 1)
    {
        SmallShell::getInstance().changeName("smash");
    }
    else
    {
        SmallShell::getInstance().changeName(args[1].c_str());
    }
}

void ShowPidCommand::execute()
{
    pid_t smashPid = SmallShell::getInstance().getSmashPid();
    cout << "smash pid is "<< to_string(smashPid) << endl;
}

void GetCurrDirCommand::execute()
{
    char cwd[1024];
    getcwd(cwd,1024);
    cout << cwd << endl;
}


void ChangeDirCommand::execute()
{
    string args[21];
    if(numOfWords(cmdLine, args) > 2)
    {
        perror("smash error: cd: too many arguments");
        return;
    }
    else if(args[1] == "-")
    {
        if (!SmallShell::getInstance().getCD())
        {
            perror("smash error: cd: OLDPWD not set");
            return;
        }
        else if (chdir(SmallShell::getInstance().getCD()) != 0)
        {
            perror("smash error: chdir failed");
            return;
        }
    }
    else
    {
        if (chdir(args[1].c_str()) != 0)
        {
            perror("smash error: chdir failed");
            return;
        }
    }
    char cwd[1024];
    SmallShell::getInstance().addCD(getcwd(cwd, 1024));
}

void ForegroundCommand::execute()
{
    string args[22];
    int argsCount = numOfWords(cmdLine, args);
    int status;
    if (argsCount == 2)
    {
        string firstArg = args[1];
        try
        {
            int pid = stoi(firstArg);
            if (pid <= 0 )
            {
                string error = "smash error:fg:job-id " + to_string(pid) + " does not exist";
                perror(error.c_str());
                return;
            }
            JobsList::JobEntry* job = jobs->getJobById(pid);
            if (!job)
            {
                string error = "smash error:fg:job-id " + to_string(pid) + " does not exist";
                perror(error.c_str());
                return;
            }
            jobs->removeJobById(job->getJobId());
            jobs->moveToFG(job);
            job->printJob();     /////////////////////////////check print forrmat
            if(waitpid(job->getPid(), &status, 0)==-1)
            {
                perror("smash error: waitpid failed");
            }
        }
        catch (exception &e)
        {
            perror("smash error: fg:invalid arguments");
        }
    }
    else if (argsCount == 1)
    {
        JobsList::JobEntry* job = jobs->getLastJob();
        if (!job)
        {
            perror("jobs list is empty");
            return;
        }
        jobs->removeJobById(job->getJobId());
        jobs->moveToFG(job);
        job->printJob();
        if(waitpid(job->getPid(), &status, 0)==-1)
        {
            perror("smash error: waitpid failed");
        }
    }
    else
    {
        perror("smash error:fg:invalid arguments");
    }
}

void BackgroundCommand::execute()
{
    string args[22];
    int argsCount = numOfWords(cmdLine, args);
    if (argsCount == 2)
    {
        string firstArg = args[1];
        try
        {
            int pid = stoi(firstArg);
            if (pid <= 0 )
            {
                string error = "smash error:bg:job-id" + to_string(pid) + "does not exist";
                perror(error.c_str());
                return;
            }
            JobsList::JobEntry* job = jobs->getJobById(pid);
            if (!job)
            {
                string error = "smash error:bg:job-id " + to_string(pid) + " does not exist";
                perror(error.c_str());
                return;
            }
            else if (job->getStat() != stopped)
            {
                string error = "smash error:bg:job-id " + to_string(pid) + " is already running in the background";
                perror(error.c_str());
                return;
            }
            jobs->moveToBG(job);
            job->printJob();
            if(kill(pid, SIGCONT)==-1)
            {
                perror("smash error: kill failed");
            }
        }
        catch (exception &e)
        {
            perror("smash error: bg:invalid arguments");
        }
    }
    else if (argsCount == 1)
    {
        JobsList::JobEntry* job = jobs->getLastStoppedJob();
        if (!job)
        {
            perror("smash error: bg:there is no stopped jobs to resume");
            return;
        }
        jobs->moveToBG(job);
        job->printJob();
        if(kill(job->getPid(), SIGCONT)==-1)
        {
            perror("smash error: kill failed");
        }
    }
    else
    {
        perror("smash error:bg:invalid arguments");
    }
}


void JobsCommand::execute()
{
    SmallShell::getInstance().getJobs()->printJobsList();
}


void SimpleCommand::execute()
{
    std::string argsTable[22];
    int argsCnt = numOfWords(cmdLine, argsTable);
    bool exists = _isBackgroundComamnd(cmdLine);
    if (exists)
    {
        if (argsTable[argsCnt-1] != "&")
            argsTable[argsCnt-1].pop_back();
        else
            argsTable[argsCnt-1] = '\0';
    }
    char** argv = new char* [argsCnt + 1];
    for(int i = 0 ; i < argsCnt ; i++)
    {
        argsTable[i]= _trim(argsTable[i]);
        char* strCopy = new char[argsTable[i].size()+1];
        std::strcpy(strCopy,argsTable[i].c_str());
        argv[i]= strCopy;
    }
    argv[argsCnt] = nullptr;
    execvp(argsTable[0].c_str(), argv);
    perror("smash error: execvp failed");
    exit(errno);
}

void ComplexCommand::execute()
{
    std::string argsTable[22];
    int argsCnt = numOfWords(cmdLine,argsTable);
    argsTable[0]="/bin/bash";
    bool exists = _isBackgroundComamnd(cmdLine);
    if (exists)
    {
        if (argsTable[argsCnt-1] != "&")
            argsTable[argsCnt-1].pop_back();
        else
            argsTable[argsCnt-1] = '\0';
    }
        int jobID = SmallShell::getInstance().getJobs()->getNextJobID();
        int pid= getpid();
        if(pid == -1)
        {
            perror("smash error: waitpid failed");
        }
        JobsList::JobEntry* job = new JobsList::JobEntry(forground, jobID, pid, cmdLine);
        job->FGjobID();
        SmallShell::getInstance().getJobs()->addToFG(job);
        if(setpgrp()==-1)
        {
            perror("smash error: setpgrp failed");
        }
        char* argv = new char [COMMAND_ARGS_MAX_LENGTH];
        strcpy(argv, cmdLine);
        _removeBackgroundSign(_trim(argv));
        strcpy(argv, _trim(argv).c_str());
        char* full_array [] = {(char*)"/bin/bash",(char*)"-c",(char*)argv, nullptr};
        execv("/bin/bash", full_array);
        perror("smash error: execv failed");
        exit(errno);
}

PipeCommand::PipeCommand(const char* cmd_line): Command(cmd_line)
{
    curCommand= nullptr;
    string cmd_s = _trim(string(cmdLine));
    char* argTable[2];
    splitByArg(cmdLine, '|', argTable);
    int fd[2];
    if (pipe(fd) == -1)
    {
        perror("smash error: pipe failed");
        return;
    }
    pid_t child1 = fork();
    if(child1 < 0)
    {
        perror("smash error: fork failed");
    }
    else if (child1 == 0)        //first command
    {
        setpgrp();
        if (cmd_s.find("|&") != string::npos)
        {
            if (dup2(fd[1], 2) == -1) {
                perror("smash error: dup2 failed");
            }
        }
        else
        {
                if (dup2(fd[1], STDOUT_FILENO) == -1)
                {
                    perror("smash error: dup2 failed");
                }
        }
        if(close(fd[0])==-1)
        {
            perror("smash error: close failed");
        }
        if(close(fd[1])==-1)
        {
            perror("smash error: close failed");
        }
        curCommand = SmallShell::getInstance().BuiltIn(argTable[0]);
        if (!curCommand)
        {
            if (cmd_s.find('*') != string::npos || cmd_s.find('?') != string::npos)
                curCommand = new ComplexCommand(argTable[0]);
            else
                curCommand = new SimpleCommand(argTable[0]);
        }
        return;
    }
    pid_t child2 = fork();
    if(child2 < 0)
    {
        perror("smash error: fork failed");
    }
    if (child2 == 0)        //second command
    {
        setpgrp();
        dup2(fd[0], 0);
        curCommand = SmallShell::getInstance().BuiltIn(argTable[1]);
        if (!curCommand)
        {
            if (cmd_s.find('*') != string::npos || cmd_s.find('?') != string::npos)
                curCommand = new ComplexCommand(argTable[1]);
            else
                curCommand = new SimpleCommand(argTable[1]);
        }
    }
    if(close(fd[0]) == -1)
    {
        perror("smash error: close failed");
    }
    if(close(fd[1]) == -1)
    {
        perror("smash error: close failed");
    }
}

void PipeCommand::execute()
{
    if (curCommand)
        curCommand->execute();
}

void SetcoreCommand::execute()
{
    std::string argTable[22];
    if(numOfWords(cmdLine,argTable)>3)
        perror("smash error: setcore: invalid argument");
    try
    {
        int jobId = stoi(argTable[1]);
        int core = stoi(argTable[2]);
        JobsList::JobEntry* job = SmallShell::getInstance().getJobs()->getJobById(jobId);
        if (!job)
        {
            std::string message = "smash error: setcore: job-id " + to_string(jobId) + " does not exist";
            perror(message.c_str());
            return;
        }
        long NumOfCores = sysconf(_SC_NPROCESSORS_ONLN);
        if (NumOfCores == -1)
        {
            perror("smash error: sysconfig failed");
            return;
        }
        if (core < 0 || core >= NumOfCores)
        {
            perror("smash error: setcore: invalid core number");
            return;
        }
        cpu_set_t cpuSet;               //Define cpu_set bit mask
        CPU_ZERO(&cpuSet);
        //Initialize it all to 0, i.e. no CPUs selected
        CPU_SET(core, &cpuSet);
        //set the bit that represents core
        if (sched_setaffinity(job->getPid(), sizeof(cpu_set_t), &cpuSet) == -1)
        {
            perror("smash error: sched_setaffinity failed");
            return;
        }
    }
    catch (...)
    {
        perror("smash error: setcore: invalid arguments");
    }
}

bool is_file_exist(const char *fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

void GetFileTypeCommand::execute()
{
    std::string output;
    std::string argTable[22];
    if(numOfWords(cmdLine,argTable)>2)
    {
        perror("smash error: gettype: invalid aruments");
    }
    if(!(is_file_exist(argTable[1].c_str())))
    {
        perror("smash error: gettype: invalid arguments");
    }
    struct stat stat_buf;

    stat(argTable[1].c_str(),&stat_buf);
     output = argTable[1] + "\'s" + " type is ";
     if(S_ISREG(stat_buf.st_mode))
            {
                output= output + "\"regular file\"";
            }
     else if (S_ISDIR(stat_buf.st_mode))
            {
                output = output + "\"directory file\"";
            }
     else if (S_ISLNK(stat_buf.st_mode))
            {
                output= output + "\"symbolic link file\"";
            }
     else if(S_ISBLK(stat_buf.st_mode))
            {
                output= output + "\"block file\"";
            }
     else if (S_ISCHR(stat_buf.st_mode))
            {
                output= output + "\"character file\"";
            }
     else if (S_ISFIFO(stat_buf.st_mode))
            {
                output= output + "\"FIFO file\"";
            }
     else if(S_ISSOCK(stat_buf.st_mode))
            {
                output= output + "\"socket file\"";
            }

        ifstream in_file(argTable[1],ios::binary);
        in_file.seekg(0,ios::end);
        int fileSize=in_file.tellg();
        output = output + " and takes up " + to_string(fileSize) + " bytes";
        cout << output << endl;
}

void ChmodCommand::execute()
{
    std::string argTable[22];
    if(numOfWords(cmdLine,argTable)>3)
    {
        perror("smash error: gettype: invalid arguments");
    }
    const char* filename =  argTable[2].c_str();
    int permissions = std::stoi(argTable[1], nullptr, 8);
    int result = chmod(filename, permissions);
    if(result < 0)
    {
        perror("smash error: chmod failed");
    }
}

void QuitCommand::execute()
{
    std::string arg[22];
    if(numOfWords(cmdLine,arg) == 1 || arg[1] != "kill")
    {
        if(kill(getpid(), SIGKILL)==-1)
        {
            perror("smash error: kill failed");
        }
    }
    jobs->removeFinishedJobs();
    int jobsNum = (int)jobs->BGround.size() + (int)jobs->Stopped.size();
    cout << "sending SIGKILL signal to " << jobsNum << " jobs" << endl;
    for(int i = 0 ; i < (int)jobs->BGround.size() ; i++){
        cout << to_string(jobs->BGround[i]->getPid()) << ": ";
        jobs->BGround[i]->printCmd();
        if(kill(jobs->BGround[i]->getPid(), SIGKILL)!=0)
        {
            perror("smash error: kill failed");
        }
    }
    for(int i = 0 ; i < (int)jobs->Stopped.size() ; i++){
        cout << to_string(jobs->Stopped[i]->getPid()) << ": ";
        jobs->Stopped[i]->printCmd();
        if(kill( this->jobs->Stopped[i]->getPid(), SIGKILL)==-1)
        {
            perror("smash error: kill failed");
        }
    }
    int pid=getpid();
    if(pid==-1)
    {
        perror("smash error: getpid failed");
    }
    if(kill(pid, SIGKILL)==-1)
    {
        perror("smash error: kill failed");
    }

}

void KillCommand::execute()
{
    std::string arg[22];
    if(numOfWords(cmdLine,arg)!=3 || arg[1].length()>3 || arg[1].length()==1)
    {
        perror("smash error: kill: invalid arguments");
    }
    try
    {
        std::string sigNum = arg[1].substr(1, arg[1].length() - 1);
        JobsList::JobEntry *job = SmallShell::getInstance().getJobs()->getJobById(stoi(arg[2]));
        if (job)
        {
            cout << "signal number " + sigNum + " was sent to pid " + to_string(job->getPid()) << endl;
            if(kill(job->getPid(), stoi(sigNum)) == -1)
            {
                perror("smash error: kill failed");
            }
            return;
        }
        else
        {
            std::string errorMessage = "smash error: kill: job-id " + arg[2] + " does not exist";
            perror(errorMessage.c_str());
        }
    }
    catch (...)
    {
        perror("smash error: kill: invalid arguments");
    }
}

void TimeoutCommand::execute()
{
    std::string argTable[22];
    string line = cmdLine;
    int i = 0;
    for ( ; i < (int)line.length() ; i++)
    {
        if (isdigit(line[i]))
            break;
    }
    string number;
    for ( ; i < (int)line.length() && isdigit(line[i]) ; i++)
        number += line[i];
    number += '\0';
    int alarmTime = stoi(number);
    //alarm(alarmTime);
    Timeout_obj* timeout = new Timeout_obj;
    timeout->timeout_pid = commandPid;
    timeout->alarm_time = time(nullptr) + alarmTime;
    timeout->cmd_line = cmdLine;
    SmallShell::getInstance().add_timeout(timeout);
}