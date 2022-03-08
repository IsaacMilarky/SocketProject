#include "../include/SplitTimerCounter.h"
#include <chrono>


SplitTimerCounter::SplitTimerCounter()
{
    minutes_ = 0;
    seconds_ = 0;
    hours_ = 0;
    milliseconds_ = 0;

    timeAdjust = 0;

    ticking = false;
}


void SplitTimerCounter::tick()
{
    
    //Sleep and then increment. Giving plenty of time for reads from other threads
    usleep(1000 - timeAdjust > 0 ? 1000 - timeAdjust : 1000);

    //Time the threading operation overhead for calibration.
    auto start_time = std::chrono::high_resolution_clock::now();

    //Should be safe enough to do before mutex since its only a read
    //Besides causes deadlock when placed after the mutex lock.
    if(!ticking)
        return;

    //Deal with minutes and seconds.
    //This is a messy if-else chain that should probably be better.
    mtx_.lock();

    if(milliseconds_ < 999)
    {
        milliseconds_++;;
    }
    else 
    {
        milliseconds_ = 0;

        if(seconds_ < 59)
        {
            seconds_++;
        }
        else
        {
            seconds_ = 0;

            if(minutes_ < 59)
            {
                minutes_++;

                //Add a second every 7 minutes to help sync
                if(minutes_ % 7 == 0)
                {
                    seconds_++;
                }
            }
            else 
            {
                minutes_ = 0;
                hours_++;
            }
            
        }
    }
    auto end_time = std::chrono::high_resolution_clock::now();

    timeAdjust = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count() / 10;
    //std::cout << "debug: " << timeAdjust << std::endl;

    mtx_.unlock();
}

void SplitTimerCounter::toggle()
{
    mtx_.lock();
    ticking = !ticking;
    mtx_.unlock();
}

unsigned short SplitTimerCounter::readMilliseconds()
{
    mtx_.lock();
    unsigned short m = milliseconds_;
    mtx_.unlock();
    return m;
}


unsigned short SplitTimerCounter::readSeconds()
{
    mtx_.lock();
    unsigned short s = seconds_;
    mtx_.unlock();
    return s;
}

unsigned long int SplitTimerCounter::readMinutes()
{
    mtx_.lock();
    unsigned long int m = minutes_;
    mtx_.unlock();
    return m;;
}

unsigned long int SplitTimerCounter::readHours()
{
    mtx_.lock();
    unsigned long int h = hours_;
    mtx_.unlock();
    return h;
}