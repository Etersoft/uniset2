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
namespace uniset
{
//--------------------------------------------------------------------------
namespace extensions
{
/*! Получение идентификатора объекта(процесса) разделяемой памяти */
uniset::ObjectId getSharedMemoryID();

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
#define dinfo if( uniset::extensions::dlog()->debugging(Debug::INFO) ) uniset::extensions::dlog()->info()
#define dwarn if( uniset::extensions::dlog()->debugging(Debug::WARN) ) uniset::extensions::dlog()->warn()
#define dcrit if( uniset::extensions::dlog()->debugging(Debug::CRIT) ) uniset::extensions::dlog()->crit()
#define dlog1 if( uniset::extensions::dlog()->debugging(Debug::LEVEL1) ) uniset::extensions::dlog()->level1()
#define dlog2 if( uniset::extensions::dlog()->debugging(Debug::LEVEL2) ) uniset::extensions::dlog()->level1()
#define dlog3 if( uniset::extensions::dlog()->debugging(Debug::LEVEL3) ) uniset::extensions::dlog()->level3()
#define dlog4 if( uniset::extensions::dlog()->debugging(Debug::LEVEL4) ) uniset::extensions::dlog()->level4()
#define dlog5 if( uniset::extensions::dlog()->debugging(Debug::LEVEL5) ) uniset::extensions::dlog()->level5()
#define dlog6 if( uniset::extensions::dlog()->debugging(Debug::LEVEL6) ) uniset::extensions::dlog()->level6()
#define dlog7 if( uniset::extensions::dlog()->debugging(Debug::LEVEL7) ) uniset::extensions::dlog()->level7()
#define dlog8 if( uniset::extensions::dlog()->debugging(Debug::LEVEL8) ) uniset::extensions::dlog()->level8()
#define dlog9 if( uniset::extensions::dlog()->debugging(Debug::LEVEL9) ) uniset::extensions::dlog()->level9()
#define dlogsys if( uniset::extensions::dlog()->debugging(Debug::SYSTEM) ) uniset::extensions::dlog()->system()
#define dlogrep if( uniset::extensions::dlog()->debugging(Debug::REPOSITORY) ) uniset::extensions::dlog()->repository()
#define dlogany uniset::extensions::dlog()->any()
// --------------------------------------------------------------------------
} // end of namespace uniset
// -------------------------------------------------------------------------

#endif // Extensions_H_
// -------------------------------------------------------------------------
