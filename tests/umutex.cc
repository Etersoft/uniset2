#include <string>
#include "Mutex.h"
#include "ThreadCreator.h"
#include "UniSetTypes.h"

using namespace std;
using namespace UniSetTypes;

uniset_mutex m;

class MyClass
{
	public:
		MyClass( const std::string name ): nm(name)
		{
			thr = new ThreadCreator<MyClass>(this, &MyClass::thread);
		}
		
		~MyClass()
		{
			delete thr;
		}
				
		void execute()
		{
			thr->start();
		}
			
	protected:
		std::string nm;

		void thread()
		{
			while(1)
			{
				{
					cerr << nm << ": before release=" << m.isRelease() << endl;
					uniset_mutex_lock l(m,5000);
					cerr << nm << ": after release=" << m.isRelease() << endl;
				}
				msleep(300);
			}
		}
				
		private:
			ThreadCreator<MyClass>* thr;
	};
		

int main( int argc, const char **argv )
{
	MyClass* mc1 = new MyClass("t1");
	MyClass* mc2 = new MyClass("t2");
	
//	m.lock();
	mc1->execute();
	msleep(200);
	mc2->execute();
	pause();
//	m.unlock();
	return 0;
}
