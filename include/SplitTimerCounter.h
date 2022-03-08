#include <boost/thread/pthread/mutex.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <boost/thread.hpp>
#include <unistd.h>

class SplitTimerCounter 
{
    //Mutex for writing to the values.
    boost::mutex mtx_;
    unsigned long int hours_;;
    unsigned long int minutes_;
    unsigned short seconds_;
    unsigned short milliseconds_;

    //Boolean for whether the stopwatch has been started or not
    bool ticking;

    //Calibration to account for threading operations.
    double timeAdjust;

    public:
        SplitTimerCounter();

        void tick();

        void toggle();

        unsigned short readMilliseconds();
        unsigned short readSeconds();;
        unsigned long int readMinutes();;
        unsigned long int readHours();;
        
};