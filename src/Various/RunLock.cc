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
 *  \brief Блокировка повторного запуска программы
 *  \author Ahton Korbin<ahtoh>, Pavel Vainerman
*/
// --------------------------------------------------------------------------
#include <dirent.h>
#include <unistd.h>
#include "RunLock.h"
#include "Configuration.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------

RunLock::RunLock()
{
}

RunLock::~RunLock()
{
}

// --------------------------------------------------------------------------
bool RunLock::isLocked( const string& name )
{
	FILE* out = fopen( string(name + ".lock" ).c_str(), "r" );

	if( out == NULL )
		return false;

	char ptr[10];
	int n = fscanf( out, "%9s", ptr );

	if( n < 1 )
	{
		fclose(out);
		return false;
	}

	DIR* d = opendir( "/proc" );
	dirent* dir;

	while((dir = readdir(d)))
	{
		if( !strcmp( ptr, dir->d_name ) )
		{
			// по хорошему здесь надо бы проверять
			// статус на зомби
			/*
								string path(dir->d_name);
								path = "/proc/" + path + "/status";
								FILE *out_st = fopen( path.c_str(), "r" );
								if(out_st)
								{
									char ptr_st[1000];
									fread(ptr_st, sizeof(ptr_st), 1, out_st);
									string str(ptr_st);
									if(str.find(exename) == str.npos)
									{
										fclose(out_st);
										break;
									}
								}
				*/
			uinfo << "(RunLock): programm " << name << " already run" << endl;

			fclose(out);
			closedir(d);
			return true;
		}
	}

	fclose(out);
	closedir(d);
	return false;
}
// --------------------------------------------------------------------------
bool RunLock::lock( const string& name )
{
	if( !isLocked(name) )
	{
		FILE* out = fopen( string(name + ".lock" ).c_str(), "w+" );

		if(out)
		{
			fprintf( out, "%d\n", getpid() );
			fclose(out);
			return true;
		}
	}

	return false;
}
// --------------------------------------------------------------------------
bool RunLock::unlock(const string& name)
{
	string fname(name + ".lock");
	FILE* out = fopen( fname.c_str(), "r" );

	if( out )
	{
		fclose(out);
		unlink(fname.c_str());
		return true;
	}

	return false;
}
// --------------------------------------------------------------------------
