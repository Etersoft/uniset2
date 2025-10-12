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
       return os << std::to_string(cmd);
    }
    // -------------------------------------------------------------------------
    std::ostream& LogServerTypes::operator<<(std::ostream& os, const LogServerTypes::lsMessage& m )
    {
        return os << "magic=" << m.magic << " cmd=" << (LogServerTypes::Command)m.cmd << " data=" << m.data;
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

            const string arg1 = checkArg(i + 1, v);

            if( arg1.empty() )
                continue;

            i++;

            const std::string filter = checkArg(i + 2, v);

            if( !filter.empty() )
                i++;

            if( c == "-s" || c == "--set" )
            {
                LogServerTypes::Command lcmd = LogServerTypes::cmdSetLevel;
                vcmd.emplace_back(lcmd, (uint32_t)Debug::value(arg1), filter);
            }
            else if( c == "-a" || c == "--add" )
            {
                LogServerTypes::Command lcmd = LogServerTypes::cmdAddLevel;
                vcmd.emplace_back(lcmd, (uint32_t)Debug::value(arg1), filter);
            }
            else if( c == "-d" || c == "--del" )
            {
                LogServerTypes::Command lcmd = LogServerTypes::cmdDelLevel;
                vcmd.emplace_back(lcmd, (uint32_t)Debug::value(arg1), filter);
            }
            else if( c == "-q" || c == "--set-verbosity" )
            {
                LogServerTypes::Command lcmd = LogServerTypes::cmdSetVerbosity;
                vcmd.emplace_back(lcmd, (uint32_t)Debug::value(arg1), filter);
            }
        }

        return vcmd;
    }
    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
namespace std
{
    std::string to_string( const uniset::LogServerTypes::Command& cmd )
    {
        switch( cmd )
        {
            case uniset::LogServerTypes::cmdSetLevel:
                return "cmdSetLevel";

            case uniset::LogServerTypes::cmdAddLevel:
                return "cmdAddLevel";

            case uniset::LogServerTypes::cmdDelLevel:
                return "cmdDelLevel";

            case uniset::LogServerTypes::cmdSetVerbosity:
                return "cmdSetVerbosity";

            case uniset::LogServerTypes::cmdRotate:
                return "cmdRotate";

            case uniset::LogServerTypes::cmdOffLogFile:
                return "cmdOffLogFile";

            case uniset::LogServerTypes::cmdOnLogFile:
                return "cmdOnLogFile";

            case uniset::LogServerTypes::cmdList:
                return "cmdList";

            case uniset::LogServerTypes::cmdFilterMode:
                return "cmdFilterMode";

            case uniset::LogServerTypes::cmdSaveLogLevel:
                return "cmdSaveLogLevel";

            case uniset::LogServerTypes::cmdRestoreLogLevel:
                return "cmdRestoreLogLevel";

            case uniset::LogServerTypes::cmdViewDefaultLogLevel:
                return "cmdViewRestoreLogLevel";

            case uniset::LogServerTypes::cmdNOP:
                return "No command(NOP)";

            default:
                return "Unknown";
        }
    }
}
// -----------------------------------------------------------------------------