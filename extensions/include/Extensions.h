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

    /*! Получение времени для подтверждения "живости" */
    int getHeartBeatTime();

    xmlNode* findNode( xmlNode* node, const std::string& snode, const std::string& field );
    
    xmlNode* getCalibrationsSection();
    
    /*! замена служебных символов в строке 
     * '\\' -> '\n'
    */
    void escape_string( std::string& s );

    /*! Загрузка калибровочной диаграммы */
    Calibration* buildCalibrationDiagram( const std::string& dname );

    extern DebugStream dlog;
}
// -------------------------------------------------------------------------
// "синтаксический сахар"..для логов
#define dinfo if( UniSetExtensions::dlog.debugging(Debug::INFO) ) UniSetExtensions::dlog
#define dwarn if( UniSetExtensions::dlog.debugging(Debug::WARN) ) UniSetExtensions::dlog
#define dcrit if( UniSetExtensions::dlog.debugging(Debug::CRIT) ) UniSetExtensions::dlog
#define dlog1 if( UniSetExtensions::dlog.debugging(Debug::LEVEL1) ) UniSetExtensions::dlog
#define dlog2 if( UniSetExtensions::dlog.debugging(Debug::LEVEL2) ) UniSetExtensions::dlog
#define dlog3 if( UniSetExtensions::dlog.debugging(Debug::LEVEL3) ) UniSetExtensions::dlog
#define dlog4 if( UniSetExtensions::dlog.debugging(Debug::LEVEL4) ) UniSetExtensions::dlog
#define dlog5 if( UniSetExtensions::dlog.debugging(Debug::LEVEL5) ) UniSetExtensions::dlog
#define dlog6 if( UniSetExtensions::dlog.debugging(Debug::LEVEL6) ) UniSetExtensions::dlog
#define dlog7 if( UniSetExtensions::dlog.debugging(Debug::LEVEL7) ) UniSetExtensions::dlog
#define dlog8 if( UniSetExtensions::dlog.debugging(Debug::LEVEL8) ) UniSetExtensions::dlog
#define dlog9 if( UniSetExtensions::dlog.debugging(Debug::LEVEL9) ) UniSetExtensions::dlog
#define dlogsys if( UniSetExtensions::dlog.debugging(Debug::SYSTEM) ) UniSetExtensions::dlog
#define dlogrep if( UniSetExtensions::dlog.debugging(Debug::REPOSITORY) ) UniSetExtensions::dlog
// -------------------------------------------------------------------------
#endif // Extensions_H_
// -------------------------------------------------------------------------
