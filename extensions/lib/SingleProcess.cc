#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include "Mutex.h"
#include "SingleProcess.h"
// --------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------------
/*! замок для блокирования совместного доступа к функции обрабтки сигналов */
static UniSetTypes::uniset_mutex signalMutex("Main::signalMutex");
static volatile sig_atomic_t procterm = 0;
static volatile sig_atomic_t doneterm = 0;
static SingleProcess* gMain = NULL;
static const int TERMINATE_TIMEOUT = 2; //  время отведенное на завершение процесса [сек]
// --------------------------------------------------------------------------------
SingleProcess::SingleProcess()
{
	gMain = this;
}

// --------------------------------------------------------------------------------
SingleProcess::~SingleProcess()
{
}
// --------------------------------------------------------------------------------
void SingleProcess::finishterm( int signo )
{
	if( !doneterm )
	{
		cerr << "(SingleProcess:finishterm): прерываем процесс завершения...!" << endl << flush;
		sigset(SIGALRM, SIG_DFL);
		gMain->set_signals(false);
		doneterm = 1;
		raise(SIGKILL);
	}
}
// ------------------------------------------------------------------------------------------
void SingleProcess::terminated( int signo )
{
	if( !signo || doneterm || !gMain || procterm )
		return;

	{
		// lock

		// на случай прихода нескольких сигналов
		uniset_mutex_lock l(signalMutex, 1000);

		if( !procterm )
		{
			procterm = 1;
			sighold(SIGALRM);
			sigset(SIGALRM, SingleProcess::finishterm);
			alarm(TERMINATE_TIMEOUT);
			sigrelse(SIGALRM);
			gMain->term(signo);
			gMain->set_signals(false);
			doneterm = 1;
			raise(signo);
		}
	}
}
// --------------------------------------------------------------------------------
void SingleProcess::set_signals( bool ask )
{
	struct sigaction act, oact;
	sigemptyset(&act.sa_mask);

	act.sa_flags = SA_RESETHAND;

	// добавляем сигналы, которые будут игнорироваться
	// при обработке сигнала
	sigaddset(&act.sa_mask, SIGINT);
	sigaddset(&act.sa_mask, SIGTERM);
	sigaddset(&act.sa_mask, SIGABRT );
	sigaddset(&act.sa_mask, SIGQUIT);
	//    sigaddset(&act.sa_mask, SIGSEGV);


	//    sigaddset(&act.sa_mask, SIGALRM);
	//    act.sa_flags = 0;
	//    act.sa_flags |= SA_RESTART;
	//    act.sa_flags |= SA_RESETHAND;
	if(ask)
		act.sa_handler = terminated;
	else
		act.sa_handler = SIG_DFL;

	sigaction(SIGINT, &act, &oact);
	sigaction(SIGTERM, &act, &oact);
	sigaction(SIGABRT, &act, &oact);
	sigaction(SIGQUIT, &act, &oact);

	//    sigaction(SIGSEGV, &act, &oact);
}
// --------------------------------------------------------------------------------
