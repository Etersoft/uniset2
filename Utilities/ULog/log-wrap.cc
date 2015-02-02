// --------------------------------------------------------------------------
#include <string>
#include "DebugStream.h"
#include "UniSetTypes.h"
#include "LogServer.h"
#include "LogServerTypes.h"
#include "Exceptions.h"
#include <sys/wait.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// -------------------------------------------------------------------------
static void print_help()
{
    printf("\n");
    printf("Usage: uniset2-logserver-wrap listen-addr listen-port PROGRAMM ARGS..\n");
    printf("\n");
}
// --------------------------------------------------------------------------
int main( int argc, char* argv[], char* envp[] )
{
    if( argc < 4 )
    {
        print_help();
        return 1;
    }
    
    string addr(argv[1]);
    int port = atoi(argv[2]);

    int pid;
    int cp[2]; /* Child to parent pipe */
    
    if( pipe(cp) < 0)
    {
        perror("Can't make pipe");
        exit(1);
    }
    
    try
    {
        switch( pid = fork() )
        {
            case -1: 
            {
                perror("Can't fork");
                exit(1);
            }
    
            case 0:
            {
                /* Child. */
                close(cp[0]);

                close( fileno(stdout) );
                  dup2(cp[1],fileno(stdout));

                close( fileno(stderr) );
                dup2(fileno(stdout),fileno(stderr));

                execvpe(argv[3], argv + 3, envp);
                perror("No exec");
                kill(getppid(), SIGQUIT);
                exit(1);
            }
            break;

            default:
            {
                /* Parent. */
                close(cp[1]);

                auto zlog = make_shared<DebugStream>();
                zlog->addLevel(Debug::ANY);

                LogServer ls(zlog);
                
                cout << "wrap: server " << addr << ":" << port << endl;
                ls.run( addr, port, true );
    
                char buf[5000];
                
                while( true )
                {
                    ssize_t r = read(cp[0], &buf, sizeof(buf)-1 );
                    if( r > 0 )
                    {
                        buf[r] = '\0';
                        (*(zlog.get())) << buf;
                    }
                }
    
                exit(0);
            }
            break;
        }
    }
    catch( const SystemError& err )
    {
        cerr << "(logserver-wrap): " << err << endl;
        return 1;
    }
    catch( const Exception& ex )
    {
        cerr << "(logserver-wrap): " << ex << endl;
        return 1;
    }
    catch(...)
    {
        cerr << "(logserver-wrap): catch(...)" << endl;
        return 1;
    }
    
    return 0;
}
// --------------------------------------------------------------------------
