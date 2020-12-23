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
#include "UniSetTypes.h"
#include "LogServerTypes.h"
#include "Debug.h"
// -------------------------------------------------------------------------
#if __BYTE_ORDER != __LITTLE_ENDIAN && __BYTE_ORDER != __BIG_ENDIAN
#error LogServerTypes: Unknown byte order!
#endif


#if __BYTE_ORDER == __LITTLE_ENDIAN
#define LE32_TO_H(x) {}
#else
#define LE32_TO_H(x) x = le32toh(x)
#endif

#if __BYTE_ORDER == __BIG_ENDIAN
#define BE32_TO_H(x) {}
#else
#define BE32_TO_H(x) x = be32toh(x)
#endif
// -------------------------------------------------------------------------
namespace uniset
{
	// -------------------------------------------------------------------------
	using namespace std;
	// -------------------------------------------------------------------------
	LogServerTypes::lsMessage::lsMessage():
		magic(MAGICNUM),
		cmd(cmdNOP),
		data(0)
	{
		std::memset(logname, 0, sizeof(logname));
#if __BYTE_ORDER == __LITTLE_ENDIAN
		_be_order = 0;
#elif __BYTE_ORDER == __BIG_ENDIAN
		_be_order = 1;
#endif
	}
	// -------------------------------------------------------------------------
	LogServerTypes::lsMessage::lsMessage( Command c, uint32_t d, const std::string& logname ):
		magic(MAGICNUM), cmd(c), data(d)
	{
#if __BYTE_ORDER == __LITTLE_ENDIAN
		_be_order = 0;
#elif __BYTE_ORDER == __BIG_ENDIAN
		_be_order = 1;
#endif
		setLogName(logname);
	}
	// -------------------------------------------------------------------------
	void LogServerTypes::lsMessage::convertFromNet() noexcept
	{
		if( _be_order )
		{
			BE32_TO_H(data);
			BE32_TO_H(magic);
		}
		else
		{
			LE32_TO_H(data);
			LE32_TO_H(magic);
		}

#if __BYTE_ORDER == __LITTLE_ENDIAN
		_be_order = 0;
#elif __BYTE_ORDER == __BIG_ENDIAN
		_be_order = 1;
#endif
	}
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
	static const std::string checkArg( size_t i, const std::vector<std::string>& v )
	{
		if( i < v.size() && (v[i])[0] != '-' )
			return v[i];

		return "";
	}
	// --------------------------------------------------------------------------

	std::vector<LogServerTypes::lsMessage> LogServerTypes::getCommands( const std::string& cmd )
	{
		vector<lsMessage> vcmd;

		auto v = uniset::explode_str(cmd, ' ');

		if( v.empty() )
			return vcmd;

		for( size_t i = 0; i < v.size(); i++ )
		{
			auto c = v[i];

			string arg1 = checkArg(i + 1, v);

			if( arg1.empty() )
				continue;

			i++;

			std::string filter = checkArg(i + 2, v);

			if( !filter.empty() )
				i++;

			if( c == "-s" || c == "--set" )
			{
				LogServerTypes::Command cmd = LogServerTypes::cmdSetLevel;
				vcmd.emplace_back(cmd, (int)Debug::value(arg1), filter);
			}
			else if( c == "-a" || c == "--add" )
			{
				LogServerTypes::Command cmd = LogServerTypes::cmdAddLevel;
				vcmd.emplace_back(cmd, (int)Debug::value(arg1), filter);
			}
			else if( c == "-d" || c == "--del" )
			{
				LogServerTypes::Command cmd = LogServerTypes::cmdDelLevel;
				vcmd.emplace_back(cmd, (uint32_t)Debug::value(arg1), filter);
			}
		}

		return vcmd;
	}
	// -------------------------------------------------------------------------
	// -------------------------------------------------------------------------

	// -------------------------------------------------------------------------
} // end of namespace uniset
