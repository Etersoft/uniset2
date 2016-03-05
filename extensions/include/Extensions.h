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
#ifndef Extensions_H_
#define Extensions_H_
// -------------------------------------------------------------------------
#include <string>
#include "UniXML.h"
#include "Debug.h"
#include "UniSetTypes.h"
#include "Calibration.h"
// -------------------------------------------------------------------------
namespace UniSetExtensions
{
	/*! Получение идентификатора объекта(процесса) разделяемой памяти */
	UniSetTypes::ObjectId getSharedMemoryID();

	xmlNode* findNode( xmlNode* node, const std::string& snode, const std::string& field );

	xmlNode* getCalibrationsSection();

	/*! замена служебных символов в строке
	 * '\\' -> '\n'
	*/
	void escape_string( std::string& s );

	/*! Загрузка калибровочной диаграммы */
	Calibration* buildCalibrationDiagram( const std::string& dname );

	void on_sigchild( int sig );

	std::shared_ptr<DebugStream> dlog();
}
// -------------------------------------------------------------------------
// "синтаксический сахар"..для логов
#define dinfo if( UniSetExtensions::dlog()->debugging(Debug::INFO) ) UniSetExtensions::dlog()->info()
#define dwarn if( UniSetExtensions::dlog()->debugging(Debug::WARN) ) UniSetExtensions::dlog()->warn()
#define dcrit if( UniSetExtensions::dlog()->debugging(Debug::CRIT) ) UniSetExtensions::dlog()->crit()
#define dlog1 if( UniSetExtensions::dlog()->debugging(Debug::LEVEL1) ) UniSetExtensions::dlog()->level1()
#define dlog2 if( UniSetExtensions::dlog()->debugging(Debug::LEVEL2) ) UniSetExtensions::dlog()->level1()
#define dlog3 if( UniSetExtensions::dlog()->debugging(Debug::LEVEL3) ) UniSetExtensions::dlog()->level3()
#define dlog4 if( UniSetExtensions::dlog()->debugging(Debug::LEVEL4) ) UniSetExtensions::dlog()->level4()
#define dlog5 if( UniSetExtensions::dlog()->debugging(Debug::LEVEL5) ) UniSetExtensions::dlog()->level5()
#define dlog6 if( UniSetExtensions::dlog()->debugging(Debug::LEVEL6) ) UniSetExtensions::dlog()->level6()
#define dlog7 if( UniSetExtensions::dlog()->debugging(Debug::LEVEL7) ) UniSetExtensions::dlog()->level7()
#define dlog8 if( UniSetExtensions::dlog()->debugging(Debug::LEVEL8) ) UniSetExtensions::dlog()->level8()
#define dlog9 if( UniSetExtensions::dlog()->debugging(Debug::LEVEL9) ) UniSetExtensions::dlog()->level9()
#define dlogsys if( UniSetExtensions::dlog()->debugging(Debug::SYSTEM) ) UniSetExtensions::dlog()->system()
#define dlogrep if( UniSetExtensions::dlog()->debugging(Debug::REPOSITORY) ) UniSetExtensions::dlog()->repository()
#define dlogany UniSetExtensions::dlog()->any()
// -------------------------------------------------------------------------
#endif // Extensions_H_
// -------------------------------------------------------------------------
