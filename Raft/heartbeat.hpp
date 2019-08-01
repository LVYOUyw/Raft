#ifndef PPCA_RAFT_HEART
#define PPCA_RAFT_HEART


#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/chrono.hpp>
#include <bits/stdc++.h>

class heartbeat
{
    public:

        heartbeat() = default;

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
            th.interrupt();
        }


        void start()
        {
            //boost::unique_lock<boost::mutex> lk(m);
            th=boost::thread([f=election, g=alive, period=period, s=state, repeat=repeat]
            {
                boost::this_thread::disable_interruption di;
                int state=s;
                do
                {
                    if (state == 1)
                    {
                        try
                        {
                            boost::this_thread::restore_interruption ri(di);
                            boost::this_thread::sleep_for(boost::chrono::milliseconds(std::rand()%150+150));
                        }
                        catch (boost::thread_interrupted)
                        {
                        //    puts("LLLL");
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
                            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
                        }
                        catch (...)
                        {
                            state = 1;
                            continue;
                        }
                    }
                } while (repeat);
            });
        }

        void stop()
        {
            repeat = 0;
            th.interrupt();
            if (th.joinable()) th.join();
        }

        ~heartbeat() {stop();}

    private:
        heartbeat(const heartbeat&);

        std::function<bool()> election = nullptr, alive = nullptr;
        boost::thread th;
        int repeat = 1;
        int state = 1;  // 1 for follower, 2 for candidate, 3 for leader
        int period;
};
#endif
