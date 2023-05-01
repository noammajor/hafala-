#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_
#include <list>
#include <vector>
#include <time.h>
#include <fstream>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
protected:
    const char* cmdLine;
public:
    Command(const char* cmd_line) : cmdLine(cmd_line){}
    virtual ~Command();
    virtual void execute() = 0;
    void printComd() const;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command{
public:
    BuiltInCommand(const char* cmd_line): Command(cmd_line){}
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
protected:
    std::fstream my_file;
    bool fdUsed;
    pid_t cmdPid;
public:
    ExternalCommand(const char* cmd_line): Command(cmd_line), fdUsed(false),my_file(NULL), cmdPid(-1){}
    virtual ~ExternalCommand();
    void execute() override;
    void changeFd(const bool append,const std::string directFile);
    pid_t getPid() const;
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
  // TODO: Add your data members
public:
    PipeCommand(const char* cmd_line): Command(cmd_line){}
    ~PipeCommand() override = default;
    void execute() override;
};

class RedirectionCommand : public Command {
    std::fstream my_file;
public:
    explicit RedirectionCommand(const char* cmd_line): Command(cmd_line){}
    ~RedirectionCommand() override = default;
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

///////////////////////////////////////////////  BuiltInCommands   ///////////////////////////////////////////////////////////

class chmpromt: public BuiltInCommand{
public:
    explicit chmpromt(const char* cmd_line) : BuiltInCommand(cmd_line){}
    virtual ~chmpromt() = default;
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
    explicit ChangeDirCommand(const char* cmd_line): BuiltInCommand(cmd_line){}
    virtual ~ChangeDirCommand() = default;
    void execute() override;
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
// TODO: Add your data members
public:
    QuitCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line){}
    virtual ~QuitCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() = default;
    void execute() override;
};

///////////////////////////////////////////////////  Jobs   ////////////////////////////////////////////////////////////////

enum status {forground, background, stopped};

class JobsList {
public:
    class JobEntry{
    private:
        time_t begin;
        status currentStatus;
        int Job_ID;
        Command* command;
        pid_t pid;
    public:
        JobEntry(status starting, int id, Command*  cmd) : begin(time(NULL)), currentStatus(starting), Job_ID(id), command(cmd){}
        ~JobEntry() = default;
        time_t getCurrentTime();
        int getJobId();
        void changeStatus(status curr);
        status getStat();
        void printJob();
        Command* getCommand();
        pid_t getPid() const;
        void FGjobID();

    };
    JobEntry* FGround;
    std::vector<JobEntry*> BGround;
    std::vector<JobEntry*> Stopped;
 // TODO: Add your data members

public:
    JobsList(): FGround(nullptr), BGround(), Stopped(){}
    ~JobsList() = default;
    void addJob(Command* cmd, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry * getLastJob();
    JobEntry *getLastStoppedJob();
    int getNextJobID ();
    void moveToFG(JobEntry* job);
    void moveToBG(JobEntry* job);
    void addToFG(JobEntry* job);
    void addToStopped(JobEntry* job);
    JobEntry* getFGjob() const;
  // TODO: Add extra methods or modify existing ones as needed
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
public:
    JobsCommand(const char* cmd_line, JobsList* jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){}
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){}
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
/* Bonus */
// TODO: Add your data members
 public:
  explicit TimeoutCommand(const char* cmd_line): BuiltInCommand(cmd_line){}
  virtual ~TimeoutCommand() {}
  void execute() override;
};

///////////////////////////////////////////////  SpecialCommands   ///////////////////////////////////////////////////////////

class ChmodCommand : public BuiltInCommand {;
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
    int jobId;
    int core;
public:
    explicit SetcoreCommand(const char* cmd_line): BuiltInCommand(cmd_line),jobId(0),core(0){}
    virtual ~SetcoreCommand() {}
    void execute() override;
};

///////////////////////////////////////////////  SmallShell   ///////////////////////////////////////////////////////////

class SmallShell {
private:
    std::string namePrompt;
    std::list<std::string> pastCd;
    JobsList* jobsList;
    static SmallShell* instance; // Guaranteed to be destroyed.

    SmallShell(): namePrompt("smash"){}
public:
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
    void addCD(std::string name);
    void removeCD();
    std::string get_name() const;
    ~SmallShell() = default;
    int listSize() const;
    void executeCommand(const char* cmd_line);
    void changeName(const char* newName);
    std::string returnPrevious() const;
    JobsList* getJobs();
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
