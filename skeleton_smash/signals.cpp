#include <iostream>
#include "signals.h"
#include "Commands.h"
#include <csignal>

using namespace std;

void ctrlZHandler(int sig_num) {
    cout << "smash: got ctrl-Z" << endl;
	JobsList::JobEntry* job = SmallShell::getInstance().getJobs()->getFGjob();
    if (!job)
        return;
    SmallShell::getInstance().getJobs()->addToFG(nullptr);
    if (job->getJobId() == -1)
    {
        SmallShell::getInstance().getJobs()->addJob(job->getCmdLine(), job->getPid(), true);
        delete job;
    }
    else
    {
        SmallShell::getInstance().getJobs()->addToStopped(job);
    }
    job->changeStatus(stopped);
    if(kill(job->getPid(), SIGSTOP) == -1)
    {
        perror("smash error: kill failed");
    }
    cout << "smash: process " << job->getPid() << " was stopped"<<endl ;
}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl;
    JobsList::JobEntry* job = SmallShell::getInstance().getJobs()->getFGjob();
    if (!job)
        return;
    SmallShell::getInstance().getJobs()->addToFG(nullptr);
    if(kill(job->getPid(), SIGKILL)==-1)
    {
        perror("smash error: kill failed");
    }
    cout << "smash: process " << to_string(job->getPid()) << " was killed" <<endl;
    delete job;
}

void alarmHandler(int sig_num) {
    cout << "smash: got an alarm" << endl;
    std::vector<Timeout_obj*> timedOut = SmallShell::getInstance().getAlarmed();
    time_t curTime = time(nullptr);
    for (int i = 0 ; i < (int)timedOut.size() ; i++)
    {
        if (timedOut[i]->timeout_pid < curTime)
        {
            cout << _trim(timedOut[i]->cmd_line) << " timed out!" << endl;
            if (timedOut[i]->timeout_pid)
                if(kill(timedOut[i]->timeout_pid, SIGKILL) == -1)
                {
                    perror("smash error: kill failed");
                }
            delete timedOut[i];
            timedOut.erase(timedOut.begin() + i);
        }
    }
    if (!timedOut.empty())
    {
        time_t min = timedOut[0]->alarm_time;
        for (Timeout_obj* obj : timedOut)
        {
            if (obj->alarm_time < min)
                min = obj->alarm_time;
        }
        time_t setAlarm = min - time(nullptr);
        alarm(setAlarm);
    }
}

