#include "timer.h"
#include <iostream>
#include <string>
#include <boost/thread.hpp>

using namespace std;

static boost::mutex mutex;

class COutJob : public CTimerJob
{
private:
    string name;
    int dur;
    
public:
    COutJob(const string& _name, int _dur = 0) : name(_name), dur(_dur) { }
    
    void Run()
    {
        mutex.lock();
        cout << name << " started" << endl;
        mutex.unlock();
        
        sleep(dur);
        
        mutex.lock();
        cout << name << " finished." << endl;
        mutex.unlock();
    }
    
    ~COutJob()
    {
        mutex.lock();
        cout << name << " destroyed." << endl;
        mutex.unlock();
    }
    
};

boost::system_time SecsFromNow(int s)
{
    return boost::get_system_time() + boost::posix_time::seconds(s);
}

int main()
{
    TimerThread::StartTimer();
    COutJob* pJob1 = new COutJob("Job1", 0);
    COutJob* pJob2 = new COutJob("Job2", 0);
    COutJob* pJob3 = new COutJob("Job3", 0);
    COutJob* pJob4 = new COutJob("Job4", 0);
    pJob1->Schedule(SecsFromNow(7));
    pJob2->Schedule(SecsFromNow(4));
    pJob3->Schedule(SecsFromNow(2));
    sleep(3);
    pJob4->Schedule(SecsFromNow(2));
    delete pJob1;
    delete pJob2;
    delete pJob3;
    sleep(5);
    delete pJob4;
    TimerThread::StopTimer();
}