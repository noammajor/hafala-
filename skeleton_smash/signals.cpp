#include <iostream>
#include <signal.h>
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
        SmallShell::getInstance().getJobs()->addJob(job->getCommand(), true);
        delete job;
    }
    else
    {
        SmallShell::getInstance().getJobs()->addToStopped(job);
    }
    kill(job->getPid(), SIGSTP);
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
  // TODO: Add your implementation
}

