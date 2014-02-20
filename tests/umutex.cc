#include <string>
#include <sstream>
#include <vector>
#include "Mutex.h"
#include "ThreadCreator.h"
#include "UniSetTypes.h"

using namespace std;
using namespace UniSetTypes;

uniset_mutex m;
uniset_rwmutex m_spin;

class MyClass
{
    public:
        MyClass( const std::string& name ): nm(name),count(0)
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

        inline std::string name(){ return nm; }
        inline int lock_count(){ return count; }
        
        
        // BAD CODE... only for test..
        inline ThreadCreator<MyClass>* get(){ return thr; }
            
    protected:
        std::string nm;

        void thread()
        {
            while(1)
            {
                {
                    uniset_mutex_lock l(m);
                    count++;
                    msleep(30);
                }
                msleep(10);
            }
        }
                
        private:
            ThreadCreator<MyClass>* thr;
            int count;
    };

class MyClassSpin
{
    public:
        MyClassSpin( const std::string& name, bool rl=false ): nm(name),readLock(rl),count(0)
        {
            thr = new ThreadCreator<MyClassSpin>(this, &MyClassSpin::thread);
        }
        
        ~MyClassSpin()
        {
            delete thr;
        }
                
        void execute()
        {
            thr->start();
        }

        inline std::string name(){ return nm; }
        inline int lock_count(){ return count; }
            
    protected:
        std::string nm;
        bool readLock;

        void thread()
        {
            while(1)
            {
                if( !readLock )
                {
//                    cerr << nm << ": before RWlock.." << endl;
                    uniset_rwmutex_wrlock l(m_spin);
                    count++;
                    msleep(30);
//                    cerr << nm << ": after RWlock.." << endl;
                }
                else
                {
//                    cerr << nm << "(readLock): before lock.." << endl;
                    uniset_rwmutex_rlock l(m_spin);
                    count++;
                    msleep(20);
//                    cerr << nm << "(readLock): after lock.." << endl;
                }
                
                msleep(20);
            }
        }
                
        private:
            ThreadCreator<MyClassSpin>* thr;
            int count;
    };
        
        
bool check_wr_lock( ost::ThreadLock& m )
{
    if( m.tryWriteLock() )
    {
        m.unlock();
        return true;
    }
    
    return false;
}

bool check_r_lock( ost::ThreadLock& m )
{
    if( m.tryReadLock() )
    {
        m.unlock();
        return true;
    }
    
    return false;
}

int main( int argc, const char **argv )
{
    try
    {
    	{
    		cout << "check timed_mutex..." << endl;
    	 	std::timed_mutex m;
    	 	
    	 	cout << " 'unlock' without 'lock'..";
    	 	m.unlock();
    	 	cout << " ok." << endl;

			cout << "try lock (lock): " << ( m.try_lock() ? "OK" : "FAIL" ) << endl;

			m.unlock();
			
    	 	m.lock();

			cout << "try lock (fail): " << ( m.try_lock() ? "FAIL" : "OK" ) << endl;
    	 	m.unlock();
    	}
    
    
    	{
    		uniset_mutex m("testmutex");
    		{
	    		uniset_mutex_lock l(m);
	    		msleep(20);
	    	}
	    	
	    	{
	    		uniset_mutex_lock l(m,100);
	    		msleep(50);
	    	}
    		
    	}
    
#if 1
        {
            uniset_rwmutex m1("mutex1");
            uniset_rwmutex m2("mutex2");
            uniset_rwmutex m3_lcopy("mutex3");
            
            cout << "m1: " << m1.name() << endl;            
            cout << "m2: " << m2.name() << endl;            
            cout << "m3: " << m3_lcopy.name() << endl;            
            
            m2 = m1;
            cout << "copy m1... m2: " << m2.name() << endl;            
            
            m1.wrlock();
            m3_lcopy = m1;
            cout << "copy m1... m3: " << m2.name() << endl;            
            cout << "m3.lock: ..." << endl;
            m3_lcopy.wrlock();
            cout << "m3.lock: wrlock OK" << endl;
        }
        
        // return 0;
#endif

#if 0
    ost::ThreadLock m;
//    cout << "test unlock.." << endl;
//    m.unlock();

    cout << "read lock.." << endl;
    m.readLock();
    cerr << "test write lock: " << (check_wr_lock(m) ? "FAIL" : "OK [0]") << endl;
    cerr << "test read lock: " << (check_r_lock(m) ? "OK [1]" : "FAIL") << endl;

    cout << "unlock.." << endl;
    m.unlock();
    cerr << "test write lock: " << (check_wr_lock(m) ? "OK [1]" : "FAIL") << endl;
    cerr << "test read lock: " << (check_r_lock(m) ? "OK [1]" : "FAIL") << endl;

    cout << "write lock.." << endl;
    m.writeLock();
    cerr << "test write lock: " << (check_wr_lock(m) ? "FAIL" : "OK [0]") << endl;
    cerr << "test read lock: " << (check_r_lock(m) ? "FAIL" : "OK [0]") << endl;

    cerr << endl << "***** uniset_rwmutex ***" << endl;
    uniset_rwmutex m1;
    cout << "read lock.." << endl;
    m1.rlock();

    cerr << "test read lock: " << endl;
    m1.rlock();
    cerr << "test read lock: OK" << endl;
    m1.unlock();
    cerr << "test write lock.." << endl;
    m1.wrlock();
    return 0;
#endif
#if 0
    uniset_mutex um;

    cout << "lock.." << endl;
    um.lock();
    cerr << "test lock: " << ( !um.isRelease() ? "OK" : "FAIL") << endl;

    um.unlock();
    cerr << "test unlock: " << (um.isRelease() ? "OK" : "FAIL") << endl;

    um.lock();
    cerr << "test second lock: " << (!um.isRelease() ? "OK" : "FAIL") << endl;
/*
    cerr << "test lock again.." << endl;
    um.lock();
    cerr << "test lock again FAIL" << endl;
*/
    um.unlock();

    cerr << "**** uniset_mutex_lock..." << endl;
    {
        uniset_mutex_lock l(um);
        cerr << "test lock: " << ( !um.isRelease() ? "OK" : "FAIL") << endl;
    }
    cerr << "test unlock: " << (um.isRelease() ? "OK" : "FAIL") << endl;
    
    {
        uniset_mutex_lock l(um);
        cerr << "test second lock: " << (!um.isRelease() ? "OK" : "FAIL") << endl;

        uniset_mutex_lock l2(um,500);
        cerr << "test wait lock: " << ( !l2.lock_ok() ? "OK" : "FAIL") << endl;
    }
    
    uniset_mutex_lock l3(um,500);
    cerr << "test wait lock: " << ( l3.lock_ok() ? "OK" : "FAIL") << endl;
    
    return 0;
#endif
    
    
    int max = 10;
    
    if( argc > 1 )
        max = UniSetTypes::uni_atoi(argv[1]);

#if 1    
    typedef std::vector<MyClass*> TVec;
     TVec tvec(max);

    for( int i=0; i<max; i++ )
    {
        ostringstream s;
        s << "t" << i;
         MyClass* t = new MyClass(s.str());
        tvec[i] = t;
         t->execute();
         msleep(50);
    }    

    cout << "TEST LOCK wait 10 sec.. (" << tvec.size() << " threads)" << endl;
    msleep(10000);
    
    cout << "TEST LOCK RESULT: " << endl;
    
    for( TVec::iterator it=tvec.begin(); it!=tvec.end(); it++ )
    {
        int c = (*it)->lock_count();
        (*it)->get()->stop();
        cout << (*it)->name() << ": locked counter: " << c << " " << ( c!=0 ? "OK":"FAIL" ) << endl;
    }
#endif

#if 1
    typedef std::vector<MyClassSpin*> TSpinVec;
     TSpinVec tsvec(max);
    for( int i=0; i<max; i++ )
    {
        ostringstream s;
        s << "t" << i;
        MyClassSpin* t = new MyClassSpin(s.str());
        tsvec[i] = t;
        t->execute();
        msleep(50);
    }   
    
    cout << "TEST WRLOCK wait 10 sec.. (" << tsvec.size() << " threads)" << endl;
    msleep(10000);
    
    cout << "TEST WRLOCK RESULT: " << endl;
    
    for( TSpinVec::iterator it=tsvec.begin(); it!=tsvec.end(); it++ )
    {
        int c = (*it)->lock_count();
        cout << (*it)->name() << ": locked counter: " << c << " " << ( c!=0 ? "OK":"FAIL" ) << endl;
    }
#endif

#if 0
    for( int i=0; i<max; i++ )
    {
        ostringstream s;
        s << "t" << i;
        MyClassSpin* t = new MyClassSpin(s.str(),true);
        t->execute();
        msleep(50);
    }   

    while(1)
    {
        {
            cerr << "(writeLock): ************* lock WAIT..." << endl;
            uniset_spin_wrlock l(m_spin);
            cerr << "(writeLock): ************* lock OK.." << endl;
        }

        msleep(15);
    }

#endif    
//    pause();

    }
    catch(omniORB::fatalException& fe)
    {
        cerr << "поймали omniORB::fatalException:" << endl;
        cerr << "  file: " << fe.file() << endl;
        cerr << "  line: " << fe.line() << endl;
        cerr << "  mesg: " << fe.errmsg() << endl;
    }    
    catch( std::exception& e )
    {
        cerr << "catch: " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "catch(...)" << endl;
    }
    
    return 0;
}
