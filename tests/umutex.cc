#include <string>
#include <sstream>
#include "Mutex.h"
#include "ThreadCreator.h"
#include "UniSetTypes.h"

using namespace std;
using namespace UniSetTypes;

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
					uniset_mutex_lock l(m);
					msleep(30);
					cerr << nm << ": after release=" << m.isRelease() << endl;
				}
				msleep(10);
			}
		}
				
		private:
			ThreadCreator<MyClass>* thr;
	};
		

int main( int argc, const char **argv )
{
	int max = 10;
	
	if( argc > 1 )
		max = UniSetTypes::uni_atoi(argv[1]);
	
	for( int i=0; i<max; i++ )
	{
		ostringstream s;
		s << "t" << i;
     	MyClass* t = new MyClass(s.str());
     	t->execute();
     	msleep(50);
	}	

	pause();
	return 0;
}
