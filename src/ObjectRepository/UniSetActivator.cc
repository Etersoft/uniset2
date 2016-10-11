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
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <sstream>
#include <fstream>

#include <condition_variable>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

// for stack trace
// --------------------
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <iomanip>
// --------------------

#include "Exceptions.h"
#include "ORepHelpers.h"
#include "UInterface.h"
#include "UniSetActivator.h"
#include "Debug.h"
#include "Configuration.h"
#include "Mutex.h"

// ------------------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// ------------------------------------------------------------------------------------------
/*
    Завершение работы организовано следующим образом:
    -------------------------------------------------
    Т.к. UniSetActivator в системе только один (singleton), он заказывает на себя
	все сигналы связанные с завершением работы, а также устанавливает обработчики на штатный выход из программы (atexit)
    и обработчик на выход по исключению (set_terminate).
    В качестве обработчика сигналов регистрируется функция activator_terminate( int signo ).
    При вызове run() (в функции work) создаётся "глобальный" поток g_term_thread, который
    засыпает до события на condition переменной g_termevent.
    Когда срабатывает обработчик сигнала, он выставляет флаг завершения (g_term=true) и вызывает
    g_termevent.notify_one(); чтобы процесс завершения начал свою работу. (см. activator_terminate).

    После этого начинается процесс завершения.
    - orb shutdown
    - uaDestroy()
    - завершение CORBA-потока (если был создан) (функция work).
    - orb destroy

    Для защиты от "зависания" во время завершения, создаётся ещё один поток g_kill_thread, который ожидает завершения
    в течение KILL_TIMEOUT и формирует сигнал SIGKILL если в течение этого времени не выставился флаг g_done = true;

    Помимио этого в процессе завершения создаётся временный поток g_fini_thread,
    который в течение TERMINATE_TIMEOUT секунд ждёт события 'g_finievent' (от процесса остановки котока для ORB).
    После чего происходит "принудительное" отключение, обработчики сигналов восстанавливаются на умолчательные и процесс
    завершается..

    В случае, если для UniSetActivator был создан отдельный поток ( run(true) ) при завершении работы программы
    срабатывает обработчик UniSetActivator::normalexit или UniSetActivator::normalterminate, которые
    делают тоже самое. Будят процесс завершения.. и дожидаются завершения его работы.

	// Обработка SIGSEGV
	Для обработки SIGSEGV выделен отдельный обработчик (activator_terminate_with_calltrace),
	который выводит stack trace и завершает работу.
	Если для ulog включён SYSTEM, то trace выводится в него для возможности (если получиться)
	увидеть его удалённо через logserver (если он запушен).
	Если не включён, то выводиться в cerr.
	Сам обработчик только выводит trace и завершает работу, восстановив обработчик SIGSEGV по умолчанию,
	без какой-либо специальной обработки завершения.

	Помимо этого, вывод stack trace сделан и для сигнала SIGABRT
*/
// ------------------------------------------------------------------------------------------
static std::shared_ptr<UniSetActivator> g_act;
static std::atomic_bool g_finished = ATOMIC_VAR_INIT(0);
static std::atomic_bool g_term = ATOMIC_VAR_INIT(0);
static std::atomic_bool g_done = ATOMIC_VAR_INIT(0);
static std::atomic_bool g_work_stopped = ATOMIC_VAR_INIT(0);
static std::atomic_int g_signo = ATOMIC_VAR_INIT(0);
static std::atomic_bool g_trace_done = ATOMIC_VAR_INIT(0);

static std::mutex              g_workmutex;
static std::mutex              g_termmutex;
static std::condition_variable g_termevent;
static std::mutex              g_finimutex;
static std::condition_variable g_finievent;
static std::mutex              g_donemutex;
static std::condition_variable g_doneevent;
static std::mutex              g_trace_donemutex;
static std::condition_variable g_trace_doneevent;
static std::shared_ptr<std::thread> g_term_thread;
static std::shared_ptr<std::thread> g_fini_thread;
static std::shared_ptr<std::thread> g_kill_thread;
static const int TERMINATE_TIMEOUT_SEC = 3; //  время отведенное на завершение процесса [сек]
static const int THREAD_TERMINATE_PAUSE = 500; // [мсек] пауза при завершении потока (см. work())
static const int KILL_TIMEOUT_SEC = 8;
static pid_t g_stacktrace_proc_pid = 0; // pid процесса делающего stack trace (для защиты от зависания)
// ------------------------------------------------------------------------------------------
// Чтобы не выделять память во время "вылета",
// выделим необходимое для stacktrace зараннее
// ----------
#define FUNCNAMESIZE 256
#define MAXFRAMES 64
// выделение специального стека заранее
// +60 - это на всякие переменные при обработке stack trace и т.п.
static char g_stack_body[(MAXFRAMES + 60)*FUNCNAMESIZE];
//static char trace_buf[10000];
static stack_t g_sigseg_stack;
static void on_stacktrace_timeout(); // поток для защиты от зависания "процесса создания stack trace"
// ------------------------------------------------------------------------------------------
// код функции printStackTrace взят с https://oroboro.com/stack-trace-on-crash/
// будет работать только под LINUX (т.к. используется backtrace)
// Как альтернативу, можно применить libunwind
// ------------------------------------------------------------------------------------------
// Выводим используя ulog() чтобы можно было удалённо увидеть через LogServer
// (хотя конечно это как повезёт и зависит от того где собственно произошла ошибка)
// ------------------------------------------------------------------------------------------
#define TRACELOG ( to_cerr ? cerr : log->system(false) )

static inline void printStackTrace()
{
	auto log = ulog();

	// если логи отключены, то и выводить не будем
	bool to_cerr = ( !log || !log->is_system() );

	// первый лог выводим с датой, а дальше уже просто
	( to_cerr ? cerr : log->system() ) << "stack trace:\n";
	TRACELOG << "-----------------------------------------" << endl;

	void* addrlist[MAXFRAMES];

	// retrieve current stack addresses
	unsigned int addrlen = backtrace(addrlist, MAXFRAMES);

	if ( addrlen == 0 )
	{
		log->system() << "...empty stack..\n";
		return;
	}

	// resolve addresses into strings containing "filename(function+address)",
	// Actually it will be ## program address function + offset
	// this array must be free()-ed
	char** symbollist = backtrace_symbols( addrlist, addrlen );

	TRACELOG << std::left;

	// iterate over the returned symbol lines. skip the first, it is the
	// address of this function.
	for ( unsigned int i = 4; i < addrlen; i++ )
	{
		Dl_info dl;

		if(!dladdr(addrlist[i], &dl))
			break;

		const char* sym = dl.dli_sname;

		int status = 0;
		char* ret = abi::__cxa_demangle( sym, NULL, 0, &status );

		if( status == 0 && ret )
			sym = ret;

		if( dl.dli_fname && sym )
		{
			TRACELOG << setw(30) << symbollist[i]
					 << " ( " << setw(40) << dl.dli_fname
					 << " ):  " << sym
					 << endl << flush;
		}
		else
		{
			TRACELOG << setw(30) << symbollist[i] << endl << flush;
		}

		if( ret )
			std::free(ret);
	}

	std::free(symbollist);
	TRACELOG << "-----------------------------------------" << endl << flush;
}
// ------------------------------------------------------------------------------------------
// за основу взят код функции отсюда https://habrahabr.ru/company/ispsystem/blog/144198/
bool gdb_print_trace()
{
	auto log = ulog();

	// если логи отключены, то и выводить будем в cerr
	bool to_cerr = ( !log || !log->is_system() );

	char pid_buf[30];
	sprintf(pid_buf, "%d", getpid());
	char name_buf[512];
	name_buf[readlink("/proc/self/exe", name_buf, 511)] = 0;

	TRACELOG << "stack trace: for " << name_buf << " pid=" << pid_buf << endl;

	int child_pid = fork();

	if( child_pid == -1 )
	{
		perror("Can't fork...");
		return false;
	}

	if( child_pid == 0 ) // CHILD
	{
		msleep(300); // пауза чтобы родитель успел подготовиться..
		dup2(2, 1); // redirect output to stderr

		if( g_act && !g_act->getAbortScript().empty() )
		{
			TRACELOG << "run script: " << g_act->getAbortScript() << " for stack trace: proc " << name_buf << " pid=" << pid_buf << endl;
			execlp(g_act->getAbortScript().c_str(), g_act->getAbortScript().c_str(), name_buf, pid_buf, NULL);
		}
		else
		{
			TRACELOG << "direct run gdb for stack trace for pid=" << pid_buf  << " name=" << name_buf << " ..." << endl;
			// приходится выводить информацию по всем потокам, т.к. мы не знаем в каком сработал сигнал
			// его надо смотреть по выводу "<signal handler called>"
			execlp("gdb", "gdb", "--batch", "-n", "-ex", "thread apply all bt", name_buf, pid_buf, NULL);
		}

		// abort(); /* If gdb failed to start */
		return false;
	}
	else // PARENT
	{
		if( g_act && !g_act->getAbortScript().empty() )
		{
			TRACELOG << "stack trace: run script " << g_act->getAbortScript() << endl;
		}

		g_stacktrace_proc_pid = child_pid;
		g_trace_done = false;
		std::thread t(on_stacktrace_timeout); // запускаем поток "защищающий" от зависания процесса создания stack trace

		waitpid(child_pid, NULL, 0);

		g_trace_done = true;
		g_trace_doneevent.notify_all();
		t.join();
	}

	TRACELOG << "-----------------------------------------" << endl << flush;
	return true;
}
// ------------------------------------------------------------------------------------------
static void on_stacktrace_timeout()
{
	ulogsys << "****** STACK TRACE guard thread start (for pid=" << g_stacktrace_proc_pid << ") ******" << endl << flush;
	std::unique_lock<std::mutex> lk(g_trace_donemutex);

	g_trace_doneevent.wait_for(lk, std::chrono::milliseconds(KILL_TIMEOUT_SEC * 1000), []()
	{
		return (g_trace_done == true);
	} );

	if( !g_trace_done )
	{
		ulogsys << "****** STACK TRACE TIMEOUT.. (kill process) *******" << endl << flush;
		kill(g_stacktrace_proc_pid, SIGKILL);
	}
	else
		ulogsys << "****** STACK TRACE guard thread finish ******" << endl << flush;
}
// ------------------------------------------------------------------------------------------
static void activator_terminate( int signo )
{
	if( g_term )
		return;

	ulogsys << "****** TERMINATE SIGNAL=" << signo << endl << flush;

	g_term = true;

	if( signo == SIGABRT )
	{
		if( g_act && !g_act->noUseGdbForStackTrace() )
		{
			if( !gdb_print_trace() )
				printStackTrace(); // пробуем сами..
		}
		else
			printStackTrace();
	}

	ulogsys << "****** TERMINATE NOTIFY...(signo=" << signo << ")" << endl << flush;
	g_signo = signo;
	g_termevent.notify_one();
}
// ------------------------------------------------------------------------------------------
static void activator_terminate_with_calltrace( int signo )
{
	if( g_act && !g_act->noUseGdbForStackTrace() )
	{
		if( !gdb_print_trace() )
			printStackTrace(); // пробуем сами..
	}
	else
		printStackTrace();

	// т.к. это SIGSEGV, то просто вылетаем по умолчанию..
	signal(signo, SIG_DFL);
	raise(signo);
}
// ------------------------------------------------------------------------------------------
void finished_thread()
{
	ulogsys << "****** FINISHED START **** " << endl << flush;
	std::unique_lock<std::mutex> lk(g_finimutex);

	if( g_finished )
		return;

	g_finished = true;

	std::unique_lock<std::mutex> lkw(g_workmutex);
	g_finievent.wait_for(lkw, std::chrono::milliseconds(TERMINATE_TIMEOUT_SEC * 1000), []()
	{
		return (g_work_stopped == true);
	} );
	ulogsys << "****** FINISHED END ****" << endl << flush;
}
// ------------------------------------------------------------------------------------------
void on_finish_timeout()
{
	std::unique_lock<std::mutex> lk(g_donemutex);

	if( g_done )
		return;

	g_doneevent.wait_for(lk, std::chrono::milliseconds(KILL_TIMEOUT_SEC * 1000), []()
	{
		return (g_done == true);
	} );

	if( !g_done )
	{
		ulogsys << "****** KILL TIMEOUT.. *******" << endl << flush;
		raise(SIGKILL);
	}

	ulogsys << "KILL THREAD: ..bye.." << endl;
}
// ------------------------------------------------------------------------------------------
void terminate_thread()
{
	ulogsys << "****** TERMINATE THREAD: STARTED *******" << endl << flush;

	{
		std::unique_lock<std::mutex> locker(g_termmutex);
		g_term = false;

		while( !g_term )
			g_termevent.wait(locker);

		// g_termmutex надо отпустить, т.к. он будет проверяться  в ~UniSetActvator
	}

	ulogsys << "****** TERMINATE THREAD: event signo=" << g_signo << endl << flush;

	{
		std::unique_lock<std::mutex> locker(g_finimutex);

		if( g_finished )
			return;
	}

	{
		std::unique_lock<std::mutex> lk(g_donemutex);
		g_done = false;
		g_kill_thread = make_shared<std::thread>(on_finish_timeout);
	}

	if( g_act )
	{
		{
			std::unique_lock<std::mutex> lk(g_finimutex);

			if( g_finished )
			{
				ulogsys << "...FINISHED ALREADY STARTED..." << endl << flush;
				return;
			}

			g_fini_thread = make_shared<std::thread>(finished_thread);
		}


		ulogsys << "(TERMINATE THREAD): call terminated.." << endl << flush;
		g_act->terminated(g_signo);

#if 0

		try
		{
			ulogsys << "TERMINATE THREAD: orb shutdown.." << endl;

			if( g_act->orb )
				g_act->orb->shutdown(true);

			ulogsys << "TERMINATE THREAD: orb shutdown ok.." << endl;
		}
		catch( const omniORB::fatalException& fe )
		{
			ulogsys << "(TERMINATE THREAD): : omniORB::fatalException:" << endl;
			ulogsys << "(TERMINATE THREAD):   file: " << fe.file() << endl;
			ulogsys << "(TERMINATE THREAD):   line: " << fe.line() << endl;
			ulogsys << "(TERMINATE THREAD):   mesg: " << fe.errmsg() << endl;
		}
		catch( const std::exception& ex )
		{
			ulogsys << "(TERMINATE THREAD): " << ex.what() << endl;
		}

#endif

		if( g_fini_thread && g_fini_thread->joinable() )
			g_fini_thread->join();

		ulogsys << "(TERMINATE THREAD): FINISHED OK.." << endl << flush;

		if( g_act &&  g_act->orb )
		{
			try
			{
				ulogsys << "(TERMINATE THREAD): destroy.." << endl;
				g_act->orb->destroy();
				ulogsys << "(TERMINATE THREAD): destroy ok.." << endl;
			}
			catch( const omniORB::fatalException& fe )
			{
				ulogsys << "(TERMINATE THREAD): : omniORB::fatalException:" << endl;
				ulogsys << "(TERMINATE THREAD):   file: " << fe.file() << endl;
				ulogsys << "(TERMINATE THREAD):   line: " << fe.line() << endl;
				ulogsys << "(TERMINATE THREAD):   mesg: " << fe.errmsg() << endl;
			}
			catch( const std::exception& ex )
			{
				ulogsys << "(TERMINATE THREAD): " << ex.what() << endl;
			}
		}

		g_act.reset();
		UniSetActivator::set_signals(false);
	}

	{
		std::unique_lock<std::mutex> lk(g_donemutex);
		g_done = true;
	}

	g_doneevent.notify_all();

	g_kill_thread->join();
	ulogsys << "(TERMINATE THREAD): ..bye.." << endl;
}
// ---------------------------------------------------------------------------
UniSetActivatorPtr UniSetActivator::inst;
// ---------------------------------------------------------------------------
UniSetActivatorPtr UniSetActivator::Instance( const UniSetTypes::ObjectId id )
{
	if( inst == nullptr )
	{
		inst = shared_ptr<UniSetActivator>( new UniSetActivator(id) );
		g_act = inst;
	}

	return inst;
}

// ---------------------------------------------------------------------------
void UniSetActivator::Destroy()
{
	inst.reset();
}
// ---------------------------------------------------------------------------
std::shared_ptr<UniSetActivator> UniSetActivator::get_aptr()
{
	return std::dynamic_pointer_cast<UniSetActivator>(get_ptr());
}
// ---------------------------------------------------------------------------
UniSetActivator::UniSetActivator( const ObjectId id ):
	UniSetManager(id),
	omDestroy(false)
{
	UniSetActivator::init();
}
// ------------------------------------------------------------------------------------------
UniSetActivator::UniSetActivator():
	UniSetManager(UniSetTypes::DefaultObjectId),
	omDestroy(false)
{
	//    thread(false);    //    отключаем поток (раз не задан id)
	UniSetActivator::init();
}

// ------------------------------------------------------------------------------------------
void UniSetActivator::init()
{
	if( getId() == DefaultObjectId )
		myname = "UniSetActivator";

	auto conf = uniset_conf();

	_noUseGdbForStackTrace = ( findArgParam("--uniset-no-use-gdb-for-stacktrace", conf->getArgc(), conf->getArgv()) != -1 );

	abortScript = conf->getArgParam("--uniset-abort-script", "");

	orb = conf->getORB();
	CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
	PortableServer::POA_var root_poa = PortableServer::POA::_narrow(obj);
	pman = root_poa->the_POAManager();
	const CORBA::PolicyList pl = conf->getPolicy();
	poa = root_poa->create_POA("my poa", pman, pl);

	if( CORBA::is_nil(poa) )
		ucrit << myname << "(init): init poa failed!!!" << endl;

	//    Чтобы подключиться к функциям завершения как можно раньше (раньше создания объектов)
	//    этот код перенесён в Configuration::uniset_init (в надежде, что uniset_init всегда вызывается одной из первых).
	//    atexit( UniSetActivator::normalexit );
	//    set_terminate( UniSetActivator::normalterminate ); // ловушка для неизвестных исключений
}

// ------------------------------------------------------------------------------------------

UniSetActivator::~UniSetActivator()
{
	if( !g_term && orbthr )
	{
		{
			std::unique_lock<std::mutex> locker(g_termmutex);
			g_term = true;
		}
		g_signo = 0;
		g_termevent.notify_one();
		ulogsys << myname << "(~UniSetActivator): wait done.." << endl;
#if 1
		//        if( g_term_thread->joinable() )
		//            g_term_thread->join();
#else
		std::unique_lock<std::mutex> locker(g_donemutex);

		while( !g_done )
			g_doneevent.wait(locker);

#endif

		ulogsys << myname << "(~UniSetActivator): wait done OK." << endl;
	}
}
// ------------------------------------------------------------------------------------------

void UniSetActivator::uaDestroy(int signo)
{
	if( omDestroy || !g_act )
		return;

	omDestroy = true;
	ulogsys << myname << "(uaDestroy): begin..." << endl;

	ulogsys << myname << "(uaDestroy): terminate... " << endl;
	term(signo);
	ulogsys << myname << "(uaDestroy): terminate ok. " << endl;

	try
	{
		stop();
	}
	catch( const omniORB::fatalException& fe )
	{
		ucrit << myname << "(uaDestroy): : поймали omniORB::fatalException:" << endl;
		ucrit << myname << "(uaDestroy):   file: " << fe.file() << endl;
		ucrit << myname << "(uaDestroy):   line: " << fe.line() << endl;
		ucrit << myname << "(uaDestroy):   mesg: " << fe.errmsg() << endl;
	}
	catch(...)
	{
		ulogsys << myname << "(uaDestroy): orb shutdown: catch... " << endl;
	}

	ulogsys << myname << "(uaDestroy): pman deactivate... " << endl;
	pman->deactivate(false, true);
	ulogsys << myname << "(uaDestroy): pman deactivate ok. " << endl;

	try
	{
		ulogsys << myname << "(uaDestroy): shutdown orb...  " << endl;

		if( orb )
			orb->shutdown(true);

		ulogsys << myname << "(uaDestroy): shutdown ok." << endl;
	}
	catch( const omniORB::fatalException& fe )
	{
		ucrit << myname << "(uaDestroy): : поймали omniORB::fatalException:" << endl;
		ucrit << myname << "(uaDestroy):   file: " << fe.file() << endl;
		ucrit << myname << "(uaDestroy):   line: " << fe.line() << endl;
		ucrit << myname << "(uaDestroy):   mesg: " << fe.errmsg() << endl;
	}
	catch( const std::exception& ex )
	{
		ucrit << myname << "(uaDestroy): " << ex.what() << endl;
	}

	ulogsys << myname << "(uaDestroy): begin..." << endl;
}

// ------------------------------------------------------------------------------------------
/*!
 *    Если thread=true то функция создает отдельный поток для обработки приходящих сообщений.
 *     И передает все ресурсы этого потока orb. А также регистрирует процесс в репозитории.
 *    \note Только после этого объект становится доступен другим процессам
 *    А далее выходит...
 *    Иначе все ресурсы основного потока передаются для обработки приходящих сообщений (и она не выходит)
 *
*/
void UniSetActivator::run( bool thread )
{
	ulogsys << myname << "(run): создаю менеджер " << endl;

	UniSetManager::initPOA( get_aptr() );

	if( getId() == UniSetTypes::DefaultObjectId )
		offThread(); // отключение потока обработки сообщений, раз не задан ObjectId

	UniSetManager::activate(); // а там вызывается активация всех подчиненных объектов и менеджеров
	active = true;

	ulogsys << myname << "(run): активизируем менеджер" << endl;
	pman->activate();
	msleep(50);

	set_signals(true);

	if( thread )
	{
		uinfo << myname << "(run): запускаемся с созданием отдельного потока...  " << endl;
		orbthr = make_shared< OmniThreadCreator<UniSetActivator> >( get_aptr(), &UniSetActivator::work);
		orbthr->start();
	}
	else
	{
		uinfo << myname << "(run): запускаемся без создания отдельного потока...  " << endl;
		work();
	}
}
// ------------------------------------------------------------------------------------------
/*!
 *    Функция останавливает работу orb и завершает поток. А так же удаляет ссылку из репозитория.
 *    \note Объект становится недоступен другим процессам
*/
void UniSetActivator::stop()
{
	if( !active )
		return;

	active = false;

	ulogsys << myname << "(stop): deactivate...  " << endl;

	deactivate();

	ulogsys << myname << "(stop): deactivate ok.  " << endl;
	ulogsys << myname << "(stop): discard request..." << endl;

	pman->discard_requests(true);

	ulogsys << myname << "(stop): discard request ok." << endl;
}

// ------------------------------------------------------------------------------------------

void UniSetActivator::work()
{
	ulogsys << myname << "(work): запускаем orb на обработку запросов..." << endl;

	try
	{
		if( orbthr )
			thid = orbthr->getTID();
		else
			thid = getpid();

		g_term_thread = make_shared<std::thread>(terminate_thread);
		omniORB::setMainThread();
		orb->run();
	}
	catch( const CORBA::SystemException& ex )
	{
		ucrit << myname << "(work): поймали CORBA::SystemException: " << ex.NP_minorString() << endl;
	}
	catch( const CORBA::Exception& ex )
	{
		ucrit << myname << "(work): поймали CORBA::Exception." << endl;
	}
	catch( const omniORB::fatalException& fe )
	{
		ucrit << myname << "(work): : поймали omniORB::fatalException:" << endl;
		ucrit << myname << "(work):   file: " << fe.file() << endl;
		ucrit << myname << "(work):   line: " << fe.line() << endl;
		ucrit << myname << "(work):   mesg: " << fe.errmsg() << endl;
	}

	ulogsys << myname << "(work): orb thread stopped!" << endl << flush;

	{
		std::unique_lock<std::mutex> lkw(g_workmutex);
		g_work_stopped = true;
	}
	g_finievent.notify_one();

	if( orbthr )
	{
		// HACK: почему-то мы должны тут застрять,
		// иначе "где-то" возникает "гонка" с потоком завершения
		// и мы получаем SIGABRT уже на самом завершении
		// (помоему как-то связано с завершением потоков)
		msleep(THREAD_TERMINATE_PAUSE); // pause();
	}
}
// ------------------------------------------------------------------------------------------
CORBA::ORB_ptr UniSetActivator::getORB()
{
	return orb;
}
// ------------------------------------------------------------------------------------------
void UniSetActivator::sysCommand( const UniSetTypes::SystemMessage* sm )
{
	switch(sm->command)
	{
		case SystemMessage::LogRotate:
		{
			ulogsys << myname << "(sysCommand): logRotate" << endl;
			auto ul = ulog();

			if( !ul )
				break;

			// переоткрываем логи
			string fname = ul->getLogFile();

			if( fname.empty() )
			{
				ul->logFile(fname.c_str(), true);
				ulogany << myname << "(sysCommand): ***************** ulog LOG ROTATE *****************" << endl;
			}
		}
		break;
	}
}
// -------------------------------------------------------------------------
void UniSetActivator::set_signals(bool ask)
{
	struct sigaction act; // = { { 0 } };
	struct sigaction oact; // = { { 0 } };
	memset(&act, 0, sizeof(act));
	memset(&act, 0, sizeof(oact));

	sigemptyset(&act.sa_mask);
	sigemptyset(&oact.sa_mask);

	// добавляем сигналы, которые будут игнорироваться
	// при обработке сигнала
	sigaddset(&act.sa_mask, SIGINT);
	sigaddset(&act.sa_mask, SIGTERM);
	sigaddset(&act.sa_mask, SIGABRT );
	sigaddset(&act.sa_mask, SIGQUIT);

	//    sigaddset(&act.sa_mask, SIGALRM);
	//    act.sa_flags = 0;
	//    act.sa_flags |= SA_RESTART;
	//    act.sa_flags |= SA_RESETHAND;
	if(ask)
		act.sa_handler = activator_terminate; // terminated;
	else
		act.sa_handler = SIG_DFL;

	sigaction(SIGINT, &act, &oact);
	sigaction(SIGTERM, &act, &oact);
	sigaction(SIGABRT, &act, &oact);
	sigaction(SIGQUIT, &act, &oact);

	// SIGSEGV отдельно
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGSEGV);
	act.sa_flags = 0;
	//	act.sa_flags |= SA_RESTART;
	act.sa_flags |= SA_RESETHAND;

#if 1
	g_sigseg_stack.ss_sp = g_stack_body;
	g_sigseg_stack.ss_flags = SS_ONSTACK;
	g_sigseg_stack.ss_size = sizeof(g_stack_body);
	assert(!sigaltstack(&g_sigseg_stack, nullptr));
	act.sa_flags |= SA_ONSTACK;
#endif

	if(ask)
		act.sa_handler = activator_terminate_with_calltrace;
	else
		act.sa_handler = SIG_DFL;

	sigaction(SIGSEGV, &act, &oact);
}

// ------------------------------------------------------------------------------------------
UniSetActivator::TerminateEvent_Signal UniSetActivator::signal_terminate_event()
{
	return s_term;
}
// ------------------------------------------------------------------------------------------
void UniSetActivator::terminated( int signo )
{
	if( !g_act )
		return;

	ulogsys << "(terminated): start.." << endl;

	try
	{
		g_act->uaDestroy(signo);
		ulogsys << "(terminated): uaDestroy ok.." << endl;
	}
	catch( const omniORB::fatalException& fe )
	{
		ulogsys << "(terminated): : поймали omniORB::fatalException:" << endl;
		ulogsys << "(terminated):   file: " << fe.file() << endl;
		ulogsys << "(terminated):   line: " << fe.line() << endl;
		ulogsys << "(terminated):   mesg: " << fe.errmsg() << endl;
	}
	catch( const std::exception& ex )
	{
		ulogsys << "(terminated): " << ex.what() << endl;
	}

	g_term = true;
	ulogsys << "terminated ok.." << endl;
}
// ------------------------------------------------------------------------------------------
void UniSetActivator::normalexit()
{
	if( !g_act )
		return;

	ulogsys << g_act->getName() << "(default exit): ..begin..." << endl << flush;

	if( g_term == false )
	{
		// прежде чем вызывать notify_one(), мы должны освободить mutex!
		{
			std::unique_lock<std::mutex> locker(g_termmutex);
			g_term = true;
			g_signo = 0;
		}
		ulogsys << "(default exit): notify terminate.." << endl << flush;
		g_termevent.notify_one();
	}

	ulogsys << "(default exit): wait done.." << endl << flush;
#if 1

	if( g_term_thread && g_term_thread->joinable() )
		g_term_thread->join();

#else

	if( g_doneevent )
	{
		std::unique_lock<std::mutex> locker(g_donemutex);

		while( !g_done )
			g_doneevent.wait(locker);
	}

#endif

	ulogsys << "(default exit): wait done OK (good bye)" << endl << flush;
}
// ------------------------------------------------------------------------------------------
void UniSetActivator::normalterminate()
{
	if( !g_act )
		return;

	ulogsys << g_act->getName() << "(default terminate): ..begin..." << endl << flush;

	if( g_term == false )
	{
		// прежде чем вызывать notify_one(), мы должны освободить mutex!
		{
			std::unique_lock<std::mutex> locker(g_termmutex);
			g_term = true;
			g_signo = 0;
		}
		ulogsys << "(default terminate): notify terminate.." << endl << flush;
		g_termevent.notify_one();
	}

	ulogsys << "(default terminate): wait done.." << endl << flush;
#if 1

	if( g_term_thread && g_term_thread->joinable() )
		g_term_thread->join();

#else

	if( g_doneevent )
	{
		std::unique_lock<std::mutex> locker(g_donemutex);

		while( !g_done )
			g_doneevent.wait(locker);
	}

#endif
	ulogsys << "(default terminate): wait done OK (good bye)" << endl << flush;
}
// ------------------------------------------------------------------------------------------
void UniSetActivator::term( int signo )
{
	ulogsys << myname << "(term): TERM" << endl;

	{
		std::unique_lock<std::mutex> locker(g_termmutex);

		if( g_term )
			return;
	}

	try
	{
		ulogsys << myname << "(term): вызываем sigterm()" << endl;
		sigterm(signo);
		s_term.emit(signo);
		ulogsys << myname << "(term): sigterm() ok." << endl;
	}
	catch( const UniSetTypes::Exception& ex )
	{
		ucrit << myname << "(term): " << ex << endl;
	}
	catch(...) {}

	ulogsys << myname << "(term): END TERM" << endl;
}
// ------------------------------------------------------------------------------------------
