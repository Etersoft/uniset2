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
		/* Create a child to run command. */
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

	            close( fileno(stderr) );   //close stderr
				dup2(fileno(stdout),fileno(stderr));

				close(fileno(stdout)); /* Close current stdout. */
  				dup2(cp[1],fileno(stdout)); /* Make stdout go to write end of pipe. */

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

				DebugStream zlog;
				zlog.addLevel(Debug::ANY);

				LogServer ls(zlog);
				
				cout << "wrap: server " << addr << ":" << port << endl;
				ls.run( addr, port, true );
	
				char buf[5000];
				
				while( true )
				{
					ssize_t r = read(cp[0], &buf, sizeof(buf)-1 );
					if( r > 0 )
					{
						cout << "***READ**" << endl;
						buf[r] = '\0';
						zlog << buf;
					}
				}
	
				exit(0);
			}
			break;
		}
	}
    catch( SystemError& err )
    {
        cerr << "(logserver-wrap): " << err << endl;
		return 1;
    }
    catch( Exception& ex )
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
