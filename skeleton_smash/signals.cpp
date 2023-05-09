#include <iostream>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    cout << "smash: got ctrl-Z" << endl;
	JobsList::JobEntry* job = SmallShell::getInstance().getJobs()->getFGjob();
    if (!job)
        return;
    SmallShell::getInstance().getJobs()->addToFG(nullptr);
    if (job->getJobId() == -1)
    {
        SmallShell::getInstance().getJobs()->addJob(job->getCmdLine(), true);
        delete job;
    }
    else
    {
        SmallShell::getInstance().getJobs()->addToStopped(job);
    }
    kill(job->getPid(), SIGSTOP);
    cout << "smash: process " << to_string(job->getPid()) << " was stopped" ;
}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl;
    JobsList::JobEntry* job = SmallShell::getInstance().getJobs()->getFGjob();
    if (!job)
        return;
    SmallShell::getInstance().getJobs()->addToFG(nullptr);
    kill(job->getPid(), SIGKILL);
    cout << "smash: process " << to_string(job->getPid()) << " was killed" ;
    delete job;
}

void alarmHandler(int sig_num) {
    cout << "smash: got an alarm" << endl;
    std::vector<Timeout_obj*> timedOut = SmallShell::getInstance().getAlarmed();
    time_t curTime = time(nullptr);
    for (Timeout_obj* obj : timedOut)
    {
        if (obj->timeout_pid < curTime)
        {
            cout << obj->cmd_line << " timed out!" << endl;
            if (obj->timeout_pid)
                kill(obj->timeout_pid, SIGKILL);
        }
    }
}

