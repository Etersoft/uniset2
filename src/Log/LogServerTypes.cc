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
// -------------------------------------------------------------------------
#include <getopt.h> // не хорошо завязыватся на getopt.. но пока так удобнее
#include "UniSetTypes.h"
#include "LogServerTypes.h"
#include "Debug.h"

// -------------------------------------------------------------------------
namespace uniset
{
	// -------------------------------------------------------------------------
	using namespace std;
	// -------------------------------------------------------------------------
	std::ostream& LogServerTypes::operator<<(std::ostream& os, LogServerTypes::Command cmd )
	{
		switch( cmd )
		{
			case LogServerTypes::cmdSetLevel:
				return os << "cmdSetLevel";

			case LogServerTypes::cmdAddLevel:
				return os << "cmdAddLevel";

			case LogServerTypes::cmdDelLevel:
				return os << "cmdDelLevel";

			case LogServerTypes::cmdRotate:
				return os << "cmdRotate";

			case LogServerTypes::cmdOffLogFile:
				return os << "cmdOffLogFile";

			case LogServerTypes::cmdOnLogFile:
				return os << "cmdOnLogFile";

			case LogServerTypes::cmdList:
				return os << "cmdList";

			case LogServerTypes::cmdFilterMode:
				return os << "cmdFilterMode";

			case LogServerTypes::cmdSaveLogLevel:
				return os << "cmdSaveLogLevel";

			case LogServerTypes::cmdRestoreLogLevel:
				return os << "cmdRestoreLogLevel";

			case LogServerTypes::cmdViewDefaultLogLevel:
				return os << "cmdViewRestoreLogLevel";

			case LogServerTypes::cmdNOP:
				return os << "No command(NOP)";

			default:
				return os << "Unknown";
		}

		return os;
	}
	// -------------------------------------------------------------------------
	std::ostream& LogServerTypes::operator<<(std::ostream& os, const LogServerTypes::lsMessage& m )
	{
		return os << " magic=" << m.magic << " cmd=" << m.cmd << " data=" << m.data;
	}
	// -------------------------------------------------------------------------
	void LogServerTypes::lsMessage::setLogName( const std::string& name )
	{
		size_t s = name.size() > MAXLOGNAME ? MAXLOGNAME : name.size();
		memcpy( &logname, name.data(), s );
		logname[s] = '\0';
	}
	// -------------------------------------------------------------------------
	static struct option longopts[] =
	{
		{ "add", required_argument, 0, 'a' },
		{ "del", required_argument, 0, 'd' },
		{ "set", required_argument, 0, 's' },
		{ "off", required_argument, 0, 'o' },
		{ "on", required_argument, 0, 'e' },
		{ "save-loglevels", required_argument, 0, 'u' },
		{ "restore-loglevels", required_argument, 0, 'y' },
		{ "rotate", optional_argument, 0, 'r' },
		{ "logfilter", required_argument, 0, 'n' },
		{ "timeout", required_argument, 0, 't' },
		{ "reconnect-delay", required_argument, 0, 'x' },
		{ NULL, 0, 0, 0 }
	};
	// --------------------------------------------------------------------------
	static const char* checkArg( int i, int argc, const char* argv[] )
	{
		if( i < argc && (argv[i])[0] != '-' )
			return argv[i];

		return 0;
	}
	// --------------------------------------------------------------------------

	std::vector<LogServerTypes::lsMessage> LogServerTypes::getCommands( const std::string& cmd )
	{
		// формируем argc, argv и проходим getopt-ом
		// пока это самый простой способ..

		auto v = uniset::explode_str(cmd,' ');
		const size_t argc = v.size()+1;
		const char** argv = new const char*[argc];

		argv[0] = " ";
		for( size_t i=1; i<argc; i++ )
			argv[i] = v[i-1].c_str(); // use strdup?

		int optindex = 0;
		int opt = 0;
		vector<lsMessage> vcmd;

		while(1)
		{
			opt = getopt_long(argc, (char**)argv, "la:d:s:n:eorx:t:uby:", longopts, &optindex);

			if( opt == -1 )
				break;

			switch (opt)
			{
				case 'a':
				{
					LogServerTypes::Command cmd = LogServerTypes::cmdAddLevel;
					std::string filter("");
					std::string d = string(optarg);
					const char* arg2 = checkArg(optind, argc, argv);

					if( arg2 )
						filter = string(arg2);

					vcmd.emplace_back(cmd, (int)Debug::value(d), filter);
				}
				break;

				case 'd':
				{
					LogServerTypes::Command cmd = LogServerTypes::cmdDelLevel;
					std::string filter("");
					std::string d = string(optarg);
					const char* arg2 = checkArg(optind, argc, argv);

					if( arg2 )
						filter = string(arg2);

					vcmd.emplace_back(cmd, (int)Debug::value(d), filter );
				}
				break;

				case 's':
				{
					LogServerTypes::Command cmd = LogServerTypes::cmdSetLevel;
					std::string filter("");
					std::string d = string(optarg);
					const char* arg2 = checkArg(optind, argc, argv);

					if( arg2 )
						filter = string(arg2);

					vcmd.emplace_back(cmd, (int)Debug::value(d), filter );
				}
				break;

				case 'l':
				{
					std::string filter("");
					const char* arg2 = checkArg(optind, argc, argv);

					if( arg2 )
						filter = string(arg2);

					vcmd.emplace_back(LogServerTypes::cmdList, 0, filter);
				}
				break;

				case 'o':
				{
					LogServerTypes::Command cmd = LogServerTypes::cmdOffLogFile;
					std::string filter("");
					const char* arg2 = checkArg(optind, argc, argv);

					if( arg2 )
						filter = string(arg2);

					vcmd.emplace_back(cmd, 0, filter);
				}
				break;

				case 'u':  // --save-loglevels
				{
					LogServerTypes::Command cmd = LogServerTypes::cmdSaveLogLevel;
					std::string filter("");
					const char* arg2 = checkArg(optind, argc, argv);

					if( arg2 )
						filter = string(arg2);

					vcmd.emplace_back(cmd, 0, filter);
				}
				break;

				case 'y':  // --restore-loglevels
				{
					LogServerTypes::Command cmd = LogServerTypes::cmdRestoreLogLevel;
					std::string filter("");
					const char* arg2 = checkArg(optind, argc, argv);

					if( arg2 )
						filter = string(arg2);

					vcmd.emplace_back(cmd, 0, filter);
				}
				break;

				case 'e':
				{
					LogServerTypes::Command cmd = LogServerTypes::cmdOnLogFile;
					std::string filter("");
					const char* arg2 = checkArg(optind, argc, argv);

					if( arg2 )
						filter = string(arg2);

					vcmd.emplace_back(cmd, 0, filter);
				}
				break;

				case 'r':
				{
					LogServerTypes::Command cmd = LogServerTypes::cmdRotate;
					std::string filter("");
					const char* arg2 = checkArg(optind, argc, argv);

					if( arg2 )
						filter = string(arg2);

					vcmd.emplace_back(cmd, 0, filter);
				}
				break;

				case '?':
				default:
				   break;
			}
		}

		delete[] argv;
		return vcmd;
	}
	// -------------------------------------------------------------------------
	// -------------------------------------------------------------------------

	// -------------------------------------------------------------------------
} // end of namespace uniset
