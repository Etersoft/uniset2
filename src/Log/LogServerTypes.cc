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
#include "LogServerTypes.h"
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
	std::ostream& LogServerTypes::operator<<(std::ostream& os, LogServerTypes::lsMessage& m )
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
} // end of namespace uniset
