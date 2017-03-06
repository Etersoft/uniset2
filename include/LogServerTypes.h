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
// -------------------------------------------------------------------------
namespace uniset
{

	namespace LogServerTypes
	{
		const unsigned int MAGICNUM = 0x20160417;
		enum Command
		{
			cmdNOP,         /*!< отсутствие команды */
			cmdSetLevel,    /*!< установить уровень вывода */
			cmdAddLevel,    /*!< добавить уровень вывода */
			cmdDelLevel,    /*!< удалить уровень вывода */
			cmdRotate,      /*!< пересоздать файл с логами */
			cmdOffLogFile,  /*!< отключить запись файла логов (если включена) */
			cmdOnLogFile,   /*!< включить запись файла логов (если была отключена) */

			// работа с логами по умолчанию
			cmdSaveLogLevel,   /*!< запомнить текущее состояние логов (для восстановления при завершении сессии или команде Restore) */
			cmdRestoreLogLevel, /*!< восстановить последнее запоненное (cmdSaveLogLevel) состояние логов (т.е. не восстанавливать как было, при завершении сессии) */

			// команды требующий ответа..
			cmdList,		/*!< вывести список контролируемых логов */
			cmdFilterMode,	/*!< включить режим работы "фильтр" - вывод только от интересующих логов, заданных в lognmae (regexp) */
			cmdViewDefaultLogLevel  /*!< вывести уровни логов сохранённых как умолчательный (cmdSaveLogLevel) */
			// cmdSetLogFile
		};

		std::ostream& operator<<(std::ostream& os, Command c );

		struct lsMessage
		{
			lsMessage(): magic(MAGICNUM), cmd(cmdNOP), data(0)
			{
				std::memset(logname, 0, sizeof(logname));
			}
			unsigned int magic;
			Command cmd;
			unsigned int data;

			static const size_t MAXLOGNAME = 30;
			char logname[MAXLOGNAME + 1]; // +1 reserverd for '\0'

			void setLogName( const std::string& name );

			// для команды 'cmdSetLogFile'
			// static const size_t MAXLOGFILENAME = 200;
			// char logfile[MAXLOGFILENAME];
		} __attribute__((packed));

		std::ostream& operator<<(std::ostream& os, lsMessage& m );
	}
	// -------------------------------------------------------------------------
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // LogServerTypes_H_
// -------------------------------------------------------------------------
