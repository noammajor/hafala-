#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_
#include <list>
#include <vector>
#include <time.h>
#include <fstream>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <sys/stat.h>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <cstring>
#include <chrono>
#include <fcntl.h>
#include <sys/types.h>
#include <csignal>
#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

std::string _trim(const std::string& s);

class Command {
protected:
    const char* cmdLine;
public:
    Command(const char* cmd_line) : cmdLine(cmd_line){}
    virtual ~Command() {}
    virtual void execute() = 0;
    void printComd() const;
    virtual bool IsLegal();
    //virtual void prepare();
    virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command{
public:
    BuiltInCommand(const char* cmd_line): Command(cmd_line){}
    virtual ~BuiltInCommand() {}
    virtual void execute() override{};
};

class ExternalCommand : public Command {
public:
    explicit ExternalCommand(const char* cmd_line): Command(cmd_line){}
    virtual ~ExternalCommand() {}
    virtual void execute();
};

class SimpleCommand : public ExternalCommand
{
public:
    SimpleCommand(const char* cmd_line) : ExternalCommand(cmd_line){}
    ~SimpleCommand() = default;
    void execute() override;
};

class ComplexCommand : public ExternalCommand
{
public:
    ComplexCommand(const char* cmd_line) : ExternalCommand(cmd_line){}
    ~ComplexCommand() = default;
    void execute() override;
};

class PipeCommand : public Command {
    Command* command1;
    Command* command2;
    pid_t pid1;
    pid_t pid2;
public:
    PipeCommand(const char* cmd_line): Command(cmd_line), command1(nullptr), command2(nullptr), pid1(-1), pid2(-1){}
    ~PipeCommand() override = default;
    void execute() override;
    //void cleanup() override;
};

class RedirectionCommand : public Command {
public:
    explicit RedirectionCommand(const char* cmd_line): Command(cmd_line){}
    ~RedirectionCommand() override = default;
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

///////////////////////////////////////////////  BuiltInCommands   ///////////////////////////////////////////////////////////

class ChpromptCommand: public BuiltInCommand {
public:
    explicit ChpromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}
    virtual ~ChpromptCommand() = default;
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
    explicit ChangeDirCommand(const char* cmd_line): BuiltInCommand(cmd_line){}
    virtual ~ChangeDirCommand() = default;
    void execute() override;
    bool IsLegal() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    explicit GetCurrDirCommand(const char* cmd_line): BuiltInCommand(cmd_line){}
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    explicit ShowPidCommand(const char* cmd_line): BuiltInCommand(cmd_line){}
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    QuitCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line),jobs(jobs){};
    virtual ~QuitCommand() {};
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    KillCommand(const char* cmd_line, JobsList* jobs):BuiltInCommand(cmd_line),jobs(jobs){};
    virtual ~KillCommand() = default;
    void execute() override;
    bool IsLegal() override;
};



///////////////////////////////////////////////////  Jobs   ////////////////////////////////////////////////////////////////

enum status {forground, background, stopped};

class JobsList {
public:
    class JobEntry{
    public:
        time_t begin;
        status currentStatus;
        int Job_ID;
        pid_t pid;
        const char* cmdLine;

        JobEntry(status starting, int id, pid_t pid, const char* cmd_line) : begin(time(NULL)), currentStatus(starting), Job_ID(id), pid(pid), cmdLine(cmd_line){}
        ~JobEntry() = default;
        time_t getCurrentTime();
        int getJobId();
        void  setID(int jobID);
        void changeStatus(status curr);
        status getStat();
        void printJob();
        void  printCmd();
        pid_t getPid() const;
        const char* getCmdLine();
        void FGjobID();

    };
    JobEntry* FGround;
    std::vector<JobEntry*> BGround;
    std::vector<JobEntry*> Stopped;


    JobsList(): FGround(nullptr), BGround(100, nullptr), Stopped(100, nullptr){}
    ~JobsList() = default;
    void addJob(const char* cmd_line, pid_t pid, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry * getLastJob();
    JobEntry *getLastStoppedJob();
    int getNextJobID ();
    void moveToBG(JobEntry* job);
    void addToFG(JobEntry* job);
    void addToStopped(JobEntry* job);
    JobEntry* getFGjob() const;
    int countJobs() const;
    void  FGtoSTP(JobsList::JobEntry* job);
    // TODO: Add extra methods or modify existing ones as needed
};

class JobsCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    JobsCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){}
    virtual ~JobsCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){}
    virtual ~ForegroundCommand() {}
    void execute() override;
    bool IsLegal() override;
};

class BackgroundCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){}
    virtual ~BackgroundCommand() {}
    void execute() override;
    bool IsLegal() override;
};

class TimeoutCommand : public BuiltInCommand {
    pid_t commandPid;
public:
    TimeoutCommand(const char* cmd_line, pid_t pid): BuiltInCommand(cmd_line), commandPid(pid){}
    virtual ~TimeoutCommand() {}
    void execute() override;
};

///////////////////////////////////////////////  SpecialCommands   ///////////////////////////////////////////////////////////

class ChmodCommand : public BuiltInCommand {
public:
    explicit ChmodCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}
    virtual ~ChmodCommand() {}
    void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand {
public:
    explicit GetFileTypeCommand(const char* cmd_line): BuiltInCommand(cmd_line){}
    virtual ~GetFileTypeCommand() {}
    void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
public:
    explicit SetcoreCommand(const char* cmd_line): BuiltInCommand(cmd_line){}
    virtual ~SetcoreCommand() {}
    void execute() override;
};

///////////////////////////////////////////////  SmallShell   ///////////////////////////////////////////////////////////


struct Timeout_obj
{
    pid_t timeout_pid;
    time_t alarm_time;
    const char* cmd_line;
};


class SmallShell {
public:
    std::string namePrompt;
    pid_t smashPid;
    const char* curCD;
    JobsList* jobsList;
    std::vector<Timeout_obj*> timeout;


    SmallShell(): namePrompt("smash"), smashPid(getpid()), curCD(nullptr), jobsList(new JobsList), timeout(){}

    static SmallShell* instance; // Guaranteed to be destroyed.

    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        if(instance == nullptr)
        {
            instance = new SmallShell();
        }// Instantiated on first use.
        return *instance;
    }
    ~SmallShell();

    pid_t getSmashPid() const;
    void addCD(const char* dir);
    const char* getCD();
    std::string get_name() const;
    void executeCommand(const char* cmd_line);
    void changeName(const char* newName);
    JobsList* getJobs();
    std::vector<Timeout_obj*> getAlarmed();
    void add_timeout(Timeout_obj* newTime);
    Command* BuiltIn(const char* cmd_line);
    bool forkExtrenal(bool setTimeout, bool runInBack, const char* cmd_line);
};

#endif //SMASH_COMMAND_H_
