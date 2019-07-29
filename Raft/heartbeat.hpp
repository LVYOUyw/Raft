#include <boost/thread/mutex.hpp>
#include <bits/stdc++.h>  

class heartbeat
{
    public:
        void start() 
        {
            boost::unique_lock<boost::mutex> lk(m);
            th=boost::thread([lk=std::move(lk), period=period,f=func] 
            {
                boost::this_thread::disable_interruption di;
                do
                {
                    try 
                    {
                        boost::this_thread::restore_interruption ri(di);
                        boost::this_thread::sleep_for(boost::chromo::milliseconds(period));
                    } 
                    catch (boost::thread_interrupted) 
                    {
                        return;
                    }
                    f();
                } while (1)
            })        
        }
        
        void stop()
        {
            th.interrupt();
            th.join();
        }
        
        void change(int period_t, function<void()> func_t) 
        {
            period = period_t;
            func = func_t;
        }
        
    private:
        boost::mutex mutex;
        std::function<void()> func =  nullptr;
        boost::thread th;
        int period = 0;
}
