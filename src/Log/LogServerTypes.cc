// -------------------------------------------------------------------------
#include "LogServerTypes.h"
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
	memcpy( &logname, name.c_str(), s );
	logname[s] = '\0';
}
// -------------------------------------------------------------------------
