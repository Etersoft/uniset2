/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// --------------------------------------------------------------------------
/*! \file
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------

#include <signal.h>
#include <unistd.h>
#include <sstream>
#include <pthread.h>
#include <time.h>

#include "PassiveTimer.h"

// ------------------------------------------------------------------------------------------
using namespace std;
//using namespace uniset;
// ------------------------------------------------------------------------------------------
void PassiveSigTimer::call(int signo, siginfo_t* evp, void* ucontext)
{
	cout << "PassiveSigTimer: callme time=" << evp->si_value.sival_int << " ms" << endl;
}

void PassiveSigTimer::callalrm(int signo)
{
	//    cout << "PassiveSigTimer: callme signo "<< signo <<endl;
}
// ------------------------------------------------------------------------------------------

PassiveSigTimer::PassiveSigTimer():
	terminated(1)
{
	init();
}
// ------------------------------------------------------------------------------------------

PassiveSigTimer::~PassiveSigTimer()
{
	terminate();
}
// ------------------------------------------------------------------------------------------
void PassiveSigTimer::init()
{
}
// ------------------------------------------------------------------------------------------
void PassiveSigTimer::terminate()
{
	if (!terminated)
	{
		t_msec = 0;
		terminated = 1;
		//        cout << "PassiveTimer("<< pid <<"): прерываю работу "<< endl;
		kill(pid, SIGALRM);
	}
}
// ------------------------------------------------------------------------------------------
bool PassiveSigTimer::wait(timeout_t timeMS)
{
	pid = getpid();

	//    struct itimerval val;
	struct sigaction action;
	sigemptyset(&action.sa_mask);

	action.sa_handler = (void(*)(int))callalrm;
	action.sa_flags = SA_RESETHAND;//SA_RESTART;

	if( sigaction(SIGALRM, &action, 0) == -1)
	{
		cerr << "PassiveSigTimer: error sigaction" << endl;
		return false;
	}


	//    if ( !terminated )
	//        terminate();
	terminated = 0;

	timeout_t sec;
	timeout_t msec;

	if (timeMS == WaitUpTime)
	{
		sec = 15 * 60; // 15min
		msec = 0;
	}
	else
	{
		sec = timeMS / 1000;
		msec = (timeMS % 1000) * 1000;
	}

	mtimer.it_value.tv_sec     = sec;
	mtimer.it_value.tv_usec    = msec;
	mtimer.it_interval.tv_sec  = 0;
	mtimer.it_interval.tv_usec = 0;
	setitimer( ITIMER_REAL, &mtimer, (struct itimerval*)0 );

	PassiveTimer::setTiming(timeMS); // вызываем для совместимости с обычным PassiveTimer-ом

	sigset_t mask, oldmask;

	sigemptyset(&mask);
	// блокируем все сигналы кроме этих
	sigaddset( &mask, SIGALRM );
	sigprocmask( SIG_BLOCK, &mask, &oldmask );

	if (timeMS == WaitUpTime)
	{
		while (!terminated)
			sigsuspend( &oldmask );
	}
	else
		sigsuspend( &oldmask );

	terminated = 1;
	sigprocmask( SIG_UNBLOCK, &mask, NULL );

	//    cout << "PassiveSigTimer: time ok"<< endl;
	return true;
}

// ------------------------------------------------------------------------------------------
/*
    struct sigaction sigv;
    struct sigevent sigx;
    struct itimerspec val;
    struct tm do_time;
    timer_t t_id;

    sigemptyset(&sigv.sa_mask);
    sigv.sa_flags = SA_SIGINFO;
    sigv.sa_sigaction = call;

    if (sigaction (SIGUSR1, &sigv, 0) == -1)
    {
        cerr << "Timer: sigaction" << endl;
        return -1;
    }

    sigx.sigev_notify = SIGEV_SIGNAL;
    sigx.sigev_signo = SIGUSR1;
    sigx.sigev_value.sival_int = timeMS;

    if ( timer_create(CLOCK_REALTIME, &sigx, &t_id) == -1 )
    {
        cerr << "Timer: timer create" << endl;
        return -1;
    }

    int sec=timeMS/1000;
    int micsec=(timeMS%1000)*1000;

    val.it_value.tv_sec    = sec;
    val.it_value.tv_nsec= micsec;
    val.it_interval.tv_sec    = sec;
    val.it_interval.tv_nsec= micsec;
    if ( timer_settime(t_id, 0, &val,0) == -1)
    {
        cerr << "settime "<< endl;
        return -1;
    }
    pause();

    if ( timer_delete( t_id )==-1)
    {
        cerr << "Timer: timer delete" << endl;
    }

    return 0;
*/
