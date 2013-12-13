#include <string>
#include <sstream>
#include "Mutex.h"
#include "ThreadCreator.h"
#include "UniSetTypes.h"
#include "modbus/TCPCheck.h"

using namespace std;
using namespace UniSetTypes;


const string ip1="localhost:2049";
const string ip2="localhost:2048";
const string ip3="192.168.77.11:2049";

uniset_mutex m;

class MyClass
{
	public:
		MyClass( const std::string& name ): nm(name)
		{
			thr = new ThreadCreator<MyClass>(this, &MyClass::thread);
		}

		~MyClass()
		{
			delete thr;
		}

		inline cctid_t start(){ return thr->start(); }
		inline void stop(){ thr->stop(); }
		inline pid_t getTID(){ return thr->getTID(); }

		// BAD code...only for tests
		inline ThreadCreator<MyClass>* mythr(){ return thr; }

	protected:
		std::string nm;


		void thread()
		{
			cout << nm << ": start thread (" << getTID() << ")" << endl;
			while(1)
			{
				ost::Thread::sleep(10000);
			}

			cout << nm << ": finish thread (" << getTID() << ")" << endl;
		}

		private:
			ThreadCreator<MyClass>* thr;
};

class MyClass2
{
	public:
		MyClass2( const std::string& name ): nm(name)
		{
			thr = new ThreadCreator<MyClass2>(this, &MyClass2::thread);
		}

		~MyClass2(){ delete thr; }

		inline cctid_t start(){ return thr->start(); }
		inline void stop(){ thr->stop(); }
		inline pid_t getTID(){ return thr->getTID(); }

	protected:
		std::string nm;
		TCPCheck tcp;

		void thread()
		{
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
			cout << nm << ": start thread (" << getTID() << ")" << endl;
			while(1)
			{
				cout << nm << " check " << ip1 << ": " << ( tcp.check(ip1,300,100) ? "OK" : "FAIL" ) << endl;
				pthread_testcancel();
				cout << nm << " check " << ip2 << ": " << ( tcp.check(ip2,300,100) ? "OK" : "FAIL" ) << endl;
				pthread_testcancel();
				cout << nm << " check " << ip3 << ": " << ( tcp.check(ip3,300,100) ? "OK" : "FAIL" ) << endl;
				pthread_testcancel();
				cout << nm << " msleep 2000" << endl;
				ost::Thread::sleep(2000);
				pthread_testcancel();
			}

			cout << nm << ": finish thread (" << getTID() << ")" << endl;
		}

		private:
			ThreadCreator<MyClass2>* thr;
};

int main( int argc, const char **argv )
{
	try
	{
		MyClass t1("Thread1");

		cout << "start1..." << endl;
		t1.start();
		msleep(30);
		cout << "TID: " << t1.getTID() << endl;

		msleep(500);

		cout << "stop1..." << endl;
		t1.stop();
		msleep(100);

		cout << "start2..." << endl;
		t1.start();

		int prior = t1.mythr()->getPriority();
		cout << "priority: " << prior << endl;

		cout << "set prior +1 [" << prior++ << "]" << endl;
		t1.mythr()->setPriority(prior);
		cout << "check priority: " << t1.mythr()->getPriority() << endl;

		prior=-2;
		cout << "set prior -2 " << endl;
		cout << "retcode=" << t1.mythr()->setPriority(prior) << endl;
		cout << "check priority: " << t1.mythr()->getPriority() << endl;

		msleep(500);

//		cout << "kill2..." << endl;
//		t1.kill(SIGUSR1);
//		msleep(100);

//		cout << "start3..." << endl;
//		t1.start();

//		pause();

		cout << "finished3..." << endl;


		TCPCheck tcp;
		cout << "check " << ip1 << ": " << ( tcp.check(ip1,300,100) ? "OK" : "FAIL" ) << endl;
		cout << "check " << ip2 << ": " << ( tcp.check(ip2,300,100) ? "OK" : "FAIL" ) << endl;
		cout << "check " << ip3 << ": " << ( tcp.check(ip3,300,100) ? "OK" : "FAIL" ) << endl;

		msleep(50);
		cout << "check finished..." << endl;


		// поток в потоке..
		MyClass2 t2("Thread2");
		cout << "thread2 start..." << endl;
		t2.start();
		msleep(6000);

		cout << "thread2 stop..." << endl;
		t2.stop();
    }
    catch( std::exception& ex )
    {
	cerr << "catch: " << ex.what() << endl;
    }

	return 0;
}
