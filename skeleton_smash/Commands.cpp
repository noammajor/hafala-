#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <cstring>
#include <filesystem>


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


///////////////////////////////////////////////  General Functions   ///////////////////////////////////////////////////////////

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

int numOfWords(const char* getNum, string* argsTable)
{
    int count = 0;
    string cur = &getNum[0];
    int index = 0;
    for(int i = 1 ; i < (int)strlen(getNum) ; i++)
    {

        if((strcmp(&getNum[i]," ") != 0 && strcmp(&getNum[i-1]," ") == 0) || (i==1 && strcmp(&getNum[0]," ")!=0 && strcmp(&getNum[1]," ")==0))
        {
            count++;
            argsTable[index++] = cur;
            cur = "";
        }
        else
            cur += &getNum[i];
        argsTable[index] = "\0";
    }
    return count;
}

char** splitByArg(const char* line, char arg)
{
    char* result[2];
    string lineString = line;
    for (int i = 0 ; i < lineString.find(arg) ; i++)
    {
        result[0] += lineString[i];
    }
    result[0] += 0;
    int j = (int)lineString.find(arg) + 1;
    for ( ; lineString[j] != 0 ; j++)
        result[1] += lineString[j];
    result[1] += 0;
    return result;
}

///////////////////////////////////////////////////////   Commands   ///////////////////////////////////////////////////////////

void Command::printComd() const
{
    cout<< cmdLine;
}


/////////////////////////////////////////////////  ExternalCommands   ///////////////////////////////////////////////////////////

pid_t ExternalCommand::getPid() const
{
    return cmdPid;
}


ExternalCommand::~ExternalCommand()
{
    if(!(my_file))
    {
        my_file.close();
    }
}

/////////////////////////////////////////////////  Jobs   ///////////////////////////////////////////////////////////

void JobsList::addJob(Command* cmd, bool isStopped)
{
    if (isStopped)
    {
        JobEntry job = JobEntry(stopped, getNextJobID(), cmd);
        Stopped.push_back(&job);
    }
    else
    {
        JobEntry job = JobEntry(background, getNextJobID(), cmd);
        Stopped.push_back(&job);
    }
}

void JobsList::printJobsList()
{
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
    BGround.clear();
    Stopped.clear();
}

void JobsList::removeFinishedJobs()
{
    for (int i = BGround.size() ; i > 0 ; i--)
    {
        if (kill(BGround[i]->getPid(), 0) == -1) {
            delete BGround[i];
            BGround.erase(BGround.begin() + i);
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
    int maxBG = BGround.back()->getJobId();
    int maxStopped = Stopped.back()->getJobId();
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
    return difftime(this->begin, time(NULL));
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
    cout << "[" << Job_ID << "] " ; ////////////////continue
    command->printComd();
    cout<<" : "<<getpid()<< getCurrentTime();
    if (currentStatus == stopped)
        cout << " (stopped)";
}

Command* JobsList::JobEntry::getCommand()
{
    return command;
}


pid_t JobsList::JobEntry::getPid() const
{
    return pid;
}

void JobsList::JobEntry::FGjobID()
{
    Job_ID = -1;
}

/////////////////////////////////////////////////  SmallShell   ///////////////////////////////////////////////////////////

int SmallShell::listSize() const
{
    return pastCd.size();
}

void SmallShell::changeName(const char* newName)
{
    string args[21];
    if(numOfWords(newName, args) < 2)
    {
        namePrompt= "smash" ;
        return;
    }
    else
    {
        namePrompt = args[1];
        return;
    }
}

std::string SmallShell::returnPrevious() const
{
    return this->pastCd.back();
}

void SmallShell::addCD(std::string name)
{
    pastCd.push_back(name);
}

void SmallShell::removeCD()
{
    pastCd.pop_back();
}

std::string SmallShell::get_name() const
{
    return namePrompt;
}

JobsList* SmallShell::getJobs()
{
    return jobsList;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (cmd_s.find("|") < cmd_s.length()) {
        return new PipeCommand(cmd_line);
    }
    if (cmd_s.find(">>") < cmd_s.length()) {
        return new PipeCommand(cmd_line); //////////////////////////////pipe command?
    }
    if (cmd_s.find(">") < cmd_s.length()) {
        return new PipeCommand(cmd_line);
    }

    if (cmd_s.find("*") < cmd_s.length() || cmd_s.find("?") < cmd_s.length()) {
        return new ComplexCommand(cmd_line);
    }

    if (firstWord.compare("chprompt") == 0) {
        return new ChmodCommand(cmd_line); ///////////////////////////
    }
    else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(cmd_line);
    }
    else if (firstWord.compare("jobs") == 0) {
        return new JobsCommand(cmd_line, jobsList);
    }
    else if (firstWord.compare("fg") == 0) {
        return new ForegroundCommand(cmd_line, jobsList);
    }
    else if (firstWord.compare("bg") == 0) {
        return new BackgroundCommand(cmd_line, jobsList);
    }
    else if (firstWord.compare("quit") == 0) {
        return new QuitCommand(cmd_line, jobsList);
    }
    else if (firstWord.compare("kill") == 0) {
        return new KillCommand(cmd_line, jobsList);
    }
    else
    {
    return new SimpleCommand(cmd_line);
    }
}

void SmallShell::executeCommand(const char *cmd_line) {
    Command* cmd = CreateCommand(cmd_line);
    cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}

/////////////////////////////////////////////////  Execute Commands   ///////////////////////////////////////////////////////////

void chmpromt::execute()
{
    string args[21];
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
    cout<<"smash pid is "<< getpid();
}

void GetCurrDirCommand::execute()
{
    char cwd[1024];
    getcwd(cwd,1024);
    cout<<cwd;
}


void ChangeDirCommand::execute()
{
    string args[21];
    ///must put the string in cut
    if(numOfWords(cmdLine, args) > 2)
    {
        perror("smash error: cd: too many arguments");
    }
    if(*cmdLine == '-')//not correct
    {
        if(SmallShell::getInstance().listSize() > 0)
        {
            if(chdir(SmallShell::getInstance().returnPrevious().c_str())==-1)
                {
                //error look at later
                }
            SmallShell::getInstance().removeCD();
        }
        else
            perror("smash error: cd: OLDPWD not set");
    }
    else /////////////////////////////////////////////////////////////////
    {
        char cwd[1024];
        getcwd(cwd, 1024);
        SmallShell::getInstance().addCD( cwd);
        if(chdir(SmallShell::getInstance().returnPrevious().c_str())==-1)
            {
                //error look at later
            }

    }
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
            job->printJob();
            waitpid(pid, &status, 0);
        }
        catch (exception &e)
        {
            perror("smash error:fg:invalid arguments");
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
        waitpid(job->getJobId(), &status, 0);
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
            kill(pid, SIGCONT);
        }
        catch (exception &e)
        {
            perror("smash error:fg:invalid arguments");
        }
    }
    else if (argsCount == 1)
    {
        JobsList::JobEntry* job = jobs->getLastStoppedJob();
        if (!job)
        {
            perror("there is no stopped jobs to resume");
            return;
        }
        jobs->moveToBG(job);
        job->printJob();
        kill(job->getPid(), SIGCONT);
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
    int argsCnt = numOfWords(cmdLine,argsTable);
    pid_t child_pid;
    int child_status;
    child_pid = fork();
    bool exists = _isBackgroundComamnd(cmdLine);
    if (exists)
    {
        if (argsTable[argsCnt-1] != "&")
            argsTable[argsCnt-1].pop_back();
        else
            argsTable[argsCnt-1] = "\0";
        SmallShell::getInstance().getJobs()->addJob(this);
    }
    if(child_pid < 0)
    {
        perror("smash error: fork failed");
    }
    else if(child_pid == 0)
    {
        pid_t pid = getpid();
        cmdPid = pid;
        setpgrp();
        execv(argsTable[0].c_str(),argsTable->c_str());
        perror("smash error: execv failed");
        exit();
    }
    else if(!(exists))
    {
        int status;
        waitpid(child_pid, &status, 0);
    }
}

void ComplexCommand::execute()
{
    std::string argsTable[22];
    int argsCnt = numOfWords(cmdLine,argsTable);
    argsTable[0]="/bin/bash";
    pid_t child_pid;
    int child_status;
    child_pid = fork();
    bool exists = _isBackgroundComamnd(cmdLine);
    if (exists)
    {
        if (argsTable[argsCnt-1] != "&")
            argsTable[argsCnt-1].pop_back();
        else
            argsTable[argsCnt-1] = "\0";
        SmallShell::getInstance().getJobs()->addJob(this);
    }
    if(child_pid < 0)
    {
        perror("smash error: fork failed");
    }
    if(child_pid == 0)
    {
        pid_t pid = getpid();
        cmdPid = pid;
        JobsList::JobEntry* job = new JobsList::JobEntry(forground, pid, this);
        job->FGjobID();
        SmallShell::getInstance().getJobs()->addToFG(job);
        setpgrp();
        execv(argsTable[0].c_str(),argsTable->c_str());
        perror("smash error: execv failed");
        exit();
    }
    else if(!(exists))
    {
        int status;
        waitpid(child_pid, &status, 0);
    }

}

void PipeCommand::execute()
{
    std::string argTable[21];
    numOfWords(cmdLine,argTable);
    SmallShell::getInstance().CreateCommand(argTable[0].c_str());
    int suc;
    int fd[2]={1,0};

    if()/// if it gets "|" arguement
    {
        suc= pipe(fd);
    }
    else
    {
        fd[0] = 2;
        suc = pipe(fd);
    }
    if(suc==-1)
    {
        perror("pipe unsuccessful");
    }
    pid_t child_pid;
    int child_status;
    child_pid = fork();
    if(child_pid == 0)
    {
        setpgrp();
        SmallShell::getInstance().CreateCommand(argTable[2].c_str()); //need to see how we send arguments
        close(fd[1]);
    }
    else
    {
        close(fd[0]);
    }
}

void SetcoreCommand::execute()
{
    std::string argTable[22];
    if(numOfWords(cmdLine,argTable)>3)
        perror("smash error: setcore: invalid argument");
    jobId = stoi(argTable[1]);        /////////////////////try and catch
    core = stoi(argTable[2]);
    JobsList::JobEntry* job;
    job = SmallShell::getInstance().getJobs()->getJobById(jobId);
    if(job == NULL)
    {
        std::string message = "smash error: setcore: job-id "+ to_string(jobId) +" does not exist";
        perror(message.c_str());
    }
    pid_t PID =job->getPid();
    int NumOfCores = sysconf(_SC_NPROCESSORS_ONLN);
    if(NumOfCores == SYSCALL_FAILED)
    {
        perror("smash error: sysconfig failed");
        return;
    }
    if(core < 0 || core >= NumOfCores)
    {
        perror("smash error: setcore: invalid core number");
    }
    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(core,&cpuSet);
    if(sched_setaffinity(job->getPid(),sizeof(cpu_set_t),&cpuSet))
    {
        perror("smash error: sched_setaffinity failed");
        return;
    }
}


void GetFileTypeCommand::execute()
{
    std::string argTable[22];
    if(numOfWords(cmdLine,argTable)>2)
    {
        perror("smash error: gettype: invalid aruments");
    }
    if(!(is_file_exist(argTable[1].c_str())))
    {
        perror("smash error: gettype: invalid aruments");
    }
    namespace fs = std::filesystem;
    std::string output;
     output = argTable[0] + "\'s" + " type is ";
    switch(argTable[1].type())
        {
            case fs::file_type::regular: output= output + "\"regular file\""; break;
            case fs::file_type::directory: output= output + "\"directory file\""; break;
            case fs::file_type::symlink: output= output + "\"symbolic link file\""; break;
            case fs::file_type::block: output= output + "\"block file\""; break;
            case fs::file_type::character:output= output + "\"character file\""; break;
            case fs::file_type::fifo: output= output + "\"FIFO file\""; break;
            case fs::file_type::socket: output= output + "\"socket file\""; break;
        }
        int fileSize= std::filesystem::file_size(argTable[0]);
        output = output + " and takes up " + fileSize + " bytes";
        cout << output;
}

bool is_file_exist(const char *fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

void ChmodCommand::execute()
{
    std::string argTable[22];
    if(numOfWords(cmdLine,argTable)>3)
    {
        perror("smash error: gettype: invalid arguments");
    }
    const char* filename = argTable[2].c_str();
    int permissions = std::stoi(argTable[1], nullptr, 8);
    int result = chmod(filename, permissions);
    if (result == -1)
        perror("smash error: chmod failed"); //////////////////////////////////// error?
}

void RedirectionCommand::execute()
{
    string s = cmdLine;
    char** lines = splitByArg(cmdLine,'>');
    bool append = false;
    if(s.find(">>"))
    {
        append = true;
        lines[1] = lines[1]+1;
    }
    pid_t child = fork();
    if(child < 0)
    {
        perror("smash error: fork failed");
    }
    if(child == 0)
    {
        int fd;
        if(append)
        {
            fd = open(lines[1], ios:: app | ios::out);
        }
        else
        {
            fd = open(lines[1],ios::trunc | ios::out);
        }
        dup2(fd,1);
        SmallShell::getInstance().executeCommand(lines[0]);
        close(fd);
        exit();
    }
    if(child > 0)
    {
        int stat;
        if( wait(&stat) < 0 )
            perror("wait failed");
        else
            chkStatus(child,stat);
    }
}
