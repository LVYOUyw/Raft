#include <boost/thread/mutex.hpp>
#include <bits/stdc++.h>  

class heartbeat
{
    public:
        
        template <class Func> 
        void initElection(Func &&f) 
        {
            election = std::forward<Func>(f);
        }
        
        template <class Func> 
        void initAlive(Func &&f) 
        {
            alive = std::forward<Func>(f);
        }
           
    
        void start() 
        {
            //boost::unique_lock<boost::mutex> lk(m);
            th=boost::thread([lk=std::move(lk), f=election, g=alive, period=period] 
            {
                boost::this_thread::disable_interruption di;
                do
                {
                    if (state == 1) 
                    {
                        try 
                        {
                            boost::this_thread::restore_interruption ri(di);
                            period=std::rand()%150+150;
                            boost::this_thread::sleep_for(boost::chromo::milliseconds(period));
                        } 
                        catch (boost::thread_interrupted) 
                        {
                            state = 1;
                            continue;
                        }
                        state = 2;      
                    }
                    else if (state == 2)
                    {
                        bool flag;
                        try
                        {
                            flag = f();
                        }
                        catch (...)
                        {
                            state = 1;
                            continue;
                        }
                        if (flag) state = 3;
                    }
                    else if (state == 3) 
                    {
                        try
                        {
                            g();
                            boost::this_thread::restore_interruption ri(di);
                            period=100;
                            boost::this_thread::sleep_for(boost::chromo::milliseconds(period));
                        }
                        catch (...) 
                        {
                            state = 1;
                            continue;
                        }
                    }
                } while (repeat)
            })        
        }

        void stop()
        {
            repeat = 0;
            th.interrupt();
            th.join();
        }
        
        ~heartbeat() {stop();}
        
    private:
        boost::mutex mutex;
        std::function<bool()> election = nullptr, alive = nullptr;
        boost::thread th;
        int repeat = 1;
        int state = 1;  // 1 for follower, 2 for candidate, 3 for leader
        int period;
};
