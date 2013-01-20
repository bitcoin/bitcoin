#include "timer.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>

#include <set>
#include <assert.h>
#include <stdio.h>

typedef std::pair<boost::system_time, CTimerJob*> tref_t;

/** Manager for scheduled timer jobs */
class CTimer {
    typedef std::set<tref_t> set_t;

    // mutex for all internal variables, AND for the private fields in
    // CTimerJobs.
    boost::mutex mutex;

    // The scheduled jobs.
    set_t jobs;

    // Waited upon by the scheduler thread when no jobs need to run.
    boost::condition_variable condTimer;

    // Waited upon by Exit() before exit.
    boost::condition_variable condExit;

    // Waited upon by Unschedule/Schedule, while the Job is already running.
    boost::condition_variable condJobDone;

    // Whether we need to shut down.
    bool fExit;

    // Whether the scheduler thread is running.
    bool fRunning;

    // @pre: job is not running or scheduled
    // @pre: lock on mutex is acquired
    void Schedule_(CTimerJob *job, const boost::system_time &time) {
        job->_scheduled = true;
        job->_time = time;
        std::pair<set_t::iterator,bool> ret = jobs.insert(std::make_pair(time, job));
        if (ret.first == jobs.begin())
            condTimer.notify_one();
    }

    // @pre: lock on mutex is acquired in lock
    // @return: whether the job needed unscheduling
    bool Unschedule_(CTimerJob *job, boost::unique_lock<boost::mutex> &lock) {
        if (job->_scheduled) {
            tref_t ref(job->_time, job);
            jobs.erase(ref);
            return true;
        }
        return false;
    }

    // @pre: no job is running
    // @pre: lock on mutex is acquired in lock
    CTimerJob *Wait_(boost::unique_lock<boost::mutex> &lock) {
        while (fExit || jobs.empty() || boost::get_system_time() < jobs.begin()->first) {
            if (fExit)
                return NULL;
            if (jobs.empty())
                condTimer.wait(lock);
            else
                condTimer.timed_wait(lock, jobs.begin()->first);
        }
        CTimerJob *job = jobs.begin()->second;
        assert(job->_scheduled == true);
        assert(job->_running == false);
        job->_scheduled = false;
        job->_running = true;
        jobs.erase(jobs.begin());
        return job;
    }

    // @pre: lock on mutex is acquired
    // @pre: job is running
    void Done_(CTimerJob *job, boost::unique_lock<boost::mutex> &lock) {
        assert(job->_running == true);
        job->_running = false;
        if (job->_autodelete && !job->_scheduled) {
            job->_autodelete = false;
            // calling delete runs Unschedule_, which needs the mutex, so give
            // it up temporarily.
            lock.unlock();
            delete job;
            lock.lock();
        }
        condJobDone.notify_one();
    }

public:
    // Construct a timer.
    CTimer() : fExit(false), fRunning(false) {}

    // Schedule a job.
    void Schedule(CTimerJob *job, const boost::system_time &time) {
        boost::unique_lock<boost::mutex> lock(mutex);

        // Once fExit is set, only Exit() modifies the state still
        if (fExit)
            return;

        // Unschedule the job, if necessary (but leave it running if it is)
        Unschedule_(job, lock);

        // (Re)schedule the job
        Schedule_(job, time);
    }

    // Unschedule a job (and wait for it to stop running, if necessary)
    bool Unschedule(CTimerJob *job) {
        boost::unique_lock<boost::mutex> lock(mutex);

        // Once fExit is set, only Exit() modifies the state still
        if (fExit)
            return false;

        // Wait until the job stops running
        while (job->_running)
            condJobDone.wait(lock);

        return Unschedule_(job, lock);
    }

    // Run the scheduler thread.
    void Run() {
        boost::unique_lock<boost::mutex> lock(mutex);
        if (fRunning)
            return;
        fRunning = true;
        while (CTimerJob *job = Wait_(lock)) {
            lock.unlock();
            try {
                job->Run();
            } catch(...) {
            }
            lock.lock();
            Done_(job, lock);
        }
        fRunning = false;
        condExit.notify_all();
    }

    // Shut down the manager.
    void Exit() {
        boost::unique_lock<boost::mutex> lock(mutex);

        // Mark manager as shutting down, and notify waiters
        fExit = true;
        condTimer.notify_all();

        // wait until Run finished
        while (fRunning)
            condExit.wait(lock);

        // Unschedule (and autodelete) remaining jobs
        for (set_t::iterator it = jobs.begin(); it != jobs.end();) {
            it->second->_scheduled = false;
            assert(it->second->_running == false);
            if (it->second->_autodelete) {
                lock.unlock();
                delete it->second;
                lock.lock();
            }
            set_t::iterator it2 = it++;
            jobs.erase(it2);
        }
    }
};

static CTimer timer;

void static thread() {
    timer.Run();
}
    
void TimerThread::StartTimer() {
    boost::thread t(thread);
}

void TimerThread::StopTimer() {
    timer.Exit();
}

void CTimerJob::Schedule(const boost::system_time &time) {
    timer.Schedule(this, time);
}

void CTimerJob::Schedule(const boost::posix_time::time_duration &dur) {
    timer.Schedule(this, boost::get_system_time() + dur);
}

bool CTimerJob::Unschedule() {
    return timer.Unschedule(this);
}

CTimerJob::~CTimerJob() {
    Unschedule();
}

CTimerJob::CTimerJob(bool autodelete) : 
    _time(boost::date_time::not_a_date_time), _running(false),
    _scheduled(false), _autodelete(autodelete) {}

CTimerJob::CTimerJob(const boost::system_time &time, bool autodelete) : 
    _time(boost::date_time::not_a_date_time), _running(false),
    _scheduled(false), _autodelete(autodelete) {
    Schedule(time);
}
