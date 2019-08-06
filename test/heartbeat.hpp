#ifndef PPCA_RAFT_HEART
#define PPCA_RAFT_HEART


#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <bits/stdc++.h>

class heartbeat
{
    public:

        heartbeat()
        {
            state = 1;
        }

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

        void interrupt()
        {
        //    printf("%d\n",state);
            th.interrupt();
        }

        void ToFollower()
        {
            state = 1;
        }

        void Run()
        {
            boost::this_thread::disable_interruption di;
            do
            {
                if (state == 1)
                {
                    try
                    {
                        boost::this_thread::restore_interruption ri(di);
                        //puts("sleep");
                        boost::this_thread::sleep_for(boost::chrono::milliseconds(std::rand()%1500+1500));
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
                        flag = election();
                    }
                    catch (boost::thread_interrupted)
                    {
                        state = 1;
                        continue;
                    }
                    if (flag) state = 3;else state = 1;
                }
                else if (state == 3)
                {
                    try
                    {
                        alive();
                        boost::this_thread::restore_interruption ri(di);
                        boost::this_thread::sleep_for(boost::chrono::milliseconds(20));
                    }
                    catch (...)
                    {
                        //puts("T");
                        state = 1;
                        continue;
                    }
                }
            } while (repeat);
        }

        void start()
        {
            boost::function0< void> f =  boost::bind(&heartbeat::Run,this);
            th=boost::thread(f);
        //    th.join();
        }

        void stop()
        {
            th.interrupt();
            if (th.joinable()) th.join();
        }

        ~heartbeat() {stop();}

    private:
        heartbeat(const heartbeat&);

        std::function<bool()> election = nullptr, alive = nullptr;
        boost::thread th;
        int repeat = 1;
        std::atomic<int> state;  // 1 for follower, 2 for candidate, 3 for leader
        int period;
};
#endif
