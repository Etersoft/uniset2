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
#ifndef LogServerTypes_H_
#define LogServerTypes_H_
// -------------------------------------------------------------------------
#include <ostream>
#include <cstring>
#include <vector>
// -------------------------------------------------------------------------
namespace uniset
{

    namespace LogServerTypes
    {
        const uint32_t MAGICNUM = 20201222;
        enum Command
        {
            cmdNOP,         /*!< отсутствие команды */
            cmdSetLevel,    /*!< установить уровень вывода */
            cmdAddLevel,    /*!< добавить уровень вывода */
            cmdDelLevel,    /*!< удалить уровень вывода */
            cmdRotate,      /*!< пересоздать файл с логами */
            cmdOffLogFile,  /*!< отключить запись файла логов (если включена) */
            cmdOnLogFile,   /*!< включить запись файла логов (если была отключена) */
            cmdSetVerbosity, /*!< установить уровень verbose */

            // работа с логами по умолчанию
            cmdSaveLogLevel,   /*!< запомнить текущее состояние логов (для восстановления при завершении сессии или команде Restore) */
            cmdRestoreLogLevel, /*!< восстановить последнее запомненное (cmdSaveLogLevel) состояние логов (т.е. не восстанавливать как было, при завершении сессии) */

            // команды требующие ответа..
            cmdList,        /*!< вывести список контролируемых логов */
            cmdFilterMode,  /*!< включить режим работы "фильтр" - вывод только от интересующих логов, заданных в logname (regexp) */
            cmdViewDefaultLogLevel, /*!< вывести уровни логов сохранённых как умолчательный (cmdSaveLogLevel) */
            // cmdSetLogFile

            // другие команды
            cmdShowLocalTime,    /*!< выводить локальное время */
            cmdShowUTCTime   /*!< выводить UTC время (по умолчанию) */
        };

        std::ostream& operator<<(std::ostream& os, Command c );

        struct lsMessage
        {
            lsMessage();
            explicit lsMessage( Command c, uint32_t d, const std::string& logname );

            uint8_t _be_order; // 1 - BE byte order, 0 - LE byte order
            uint32_t magic;
            uint32_t data;
            uint8_t cmd;

            static const size_t MAXLOGNAME = 120;
            char logname[MAXLOGNAME + 1]; // +1 reserverd for '\0'

            void setLogName( const std::string& name );
            void convertFromNet() noexcept;

            // для команды 'cmdSetLogFile'
            // static const size_t MAXLOGFILENAME = 200;
            // char logfile[MAXLOGFILENAME];
        } __attribute__((packed));

        std::ostream& operator<<(std::ostream& os, const lsMessage& m );

        /*! Разбор строки на команды:
         *
         * [-a | --add] info,warn,crit,... [logfilter] - Add log levels.
         * [-d | --del] info,warn,crit,... [logfilter] - Delete log levels.
         * [-s | --set] info,warn,crit,... [logfilter] - Set log levels.
         *
         * 'logfilter' - regexp for name of log. Default: ALL logs
         */
        std::vector<lsMessage> getCommands( const std::string& cmd );
    }
    // -------------------------------------------------------------------------
} // end of uniset namespace
// -------------------------------------------------------------------------
namespace std
{
    std::string to_string( const uniset::LogServerTypes::Command& c );
}
// -----------------------------------------------------------------------------

#endif // LogServerTypes_H_
// -------------------------------------------------------------------------
