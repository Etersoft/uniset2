#include <string>
#include <sstream>
#include <vector>
#include "Mutex.h"
#include "ThreadCreator.h"
#include "UniSetTypes.h"

using namespace std;
using namespace uniset2;

uniset_mutex m;
uniset_rwmutex m_rw;

class MyClass
{
    public:
        MyClass( const std::string& name ): nm(name), term(false), count(0)
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

        void terminate()
        {
            term = true;
        }

        inline std::string name()
        {
            return nm;
        }
        inline int lock_count()
        {
            return count;
        }


        // BAD CODE... only for test..
        inline ThreadCreator<MyClass>* get()
        {
            return thr;
        }

    protected:
        std::string nm;
        std::atomic_bool term;

        void thread()
        {
            while(!term)
            {
                {
                    uniset_mutex_lock l(m);
                    count++;
                }
            }
        }

    private:
        ThreadCreator<MyClass>* thr;
        int count;
};

class MyClassSpin
{
    public:
        MyClassSpin( const std::string& name, bool rl = false ): nm(name), readLock(rl), term(false), count(0)
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

        void terminate()
        {
            term = true;
        }


        inline std::string name()
        {
            return nm;
        }
        inline int lock_count()
        {
            return count;
        }

        // BAD CODE... only for test..
        inline ThreadCreator<MyClassSpin>* get()
        {
            return thr;
        }

    protected:
        std::string nm;
        bool readLock;
        std::atomic_bool term;

        void thread()
        {
            while(!term)
            {
                if( !readLock )
                {
                    uniset_rwmutex_wrlock l(m_rw);
                    count++;
                }
                else
                {
                    uniset_rwmutex_rlock l(m_rw);
                    count++;
                }

                //msleep(20);
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

int main( int argc, const char** argv )
{
    try
    {
#if 0
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
                uniset_mutex_lock l(m, 100);
                msleep(50);
            }

        }
#endif
#if 0
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

            uniset_mutex_lock l2(um, 500);
            cerr << "test wait lock: " << ( !l2.lock_ok() ? "OK" : "FAIL") << endl;
        }

        uniset_mutex_lock l3(um, 500);
        cerr << "test wait lock: " << ( l3.lock_ok() ? "OK" : "FAIL") << endl;

        return 0;
#endif


        int max = 10;

        if( argc > 1 )
            max = uniset2::uni_atoi(argv[1]);

#if 1
        typedef std::vector<MyClass*> TVec;
        TVec tvec(max);

        for( int i = 0; i < max; i++ )
        {
            ostringstream s;
            s << "t" << i;
            MyClass* t = new MyClass(s.str());
            tvec[i] = t;
            t->execute();
            msleep(50);
        }

        cout << "TEST MUTEX LOCK wait 10 sec.. (" << tvec.size() << " threads)" << endl;
        msleep(10000);

        cout << "TEST MUTEX LOCK RESULT: " << endl;

        for( const auto& it: tvec )
        {
            int c = it->lock_count();
            it->terminate();

            if( it->get()->isRunning() )
                it->get()->join();

            //(*it)->get()->stop();
            cout << it->name() << ": locked counter: " << (c / 10) << " " << ( c != 0 ? "OK" : "FAIL" ) << endl;
        }

#endif

#if 1
        typedef std::vector<MyClassSpin*> TSpinVec;
        TSpinVec tsvec(max);
        int half = 3; // max/2;

        for( int i = 0; i < max; i++ )
        {
            ostringstream s;
            bool r = false;
#if 1

            if( i >= half )
            {
                r = true;
                s << "(R)";
            }
            else
#endif
                s << "(W)";

            s << "t" << i;

            MyClassSpin* t = new MyClassSpin(s.str(), r);
            tsvec[i] = t;
            t->execute();
            msleep(20);
        }

        cout << "TEST RWMUTEX LOCK wait 10 sec.. (" << tsvec.size() << " threads)" << endl;
        msleep(10000);

        cout << "TEST RWMUTEX LOCK RESULT: " << endl;

        for( const auto& it: tsvec )
        {
            int c = it->lock_count();
            it->terminate();

            if( it->get()->isRunning() )
                it->get()->join();

            cout << it->name() << ": locked counter: " << (c / 10) << " " << ( c != 0 ? "OK" : "FAIL" ) << endl;
        }

#endif

#if 0
        typedef std::vector<MyClassSpin*> TSpinVec2;
        TSpinVec2 tsvec2(max);

        for( int i = 0; i < max; i++ )
        {
            ostringstream s;
            s << "t" << i;
            MyClassSpin* t = new MyClassSpin(s.str(), true);

            tsvec[i] = t;
            t->execute();
            msleep(50);
        }

        std::atomic_int cnt(0);
        std::atomic_int num(10);

        while( --num )
        {
            {
                // cerr << "(writeLock): ************* lock WAIT..." << endl;
                uniset_rwmutex_wrlock l(m_rw);
                cnt++;
                // cerr << "(writeLock): ************* lock OK.." << endl;
            }

            msleep(10);
        }

        cout << "WRITE LOCK: " << cnt << endl;

        for( auto& it : tsvec2 )
        {
            int c = it->lock_count();
            it->terminate();

            if( it->get()->isRunning() )
                it->get()->join();

            //        cout << it->name() << ": locked counter: " << c << " " << ( c!=0 ? "OK":"FAIL" ) << endl;
        }


#endif
        //    pause();

    }
    catch( const omniORB::fatalException& fe )
    {
        cerr << "поймали omniORB::fatalException:" << endl;
        cerr << "  file: " << fe.file() << endl;
        cerr << "  line: " << fe.line() << endl;
        cerr << "  mesg: " << fe.errmsg() << endl;
    }
    catch( const std::exception& e )
    {
        cerr << "catch: " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "catch(...)" << endl;
    }

    return 0;
}
