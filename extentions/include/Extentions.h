// $Id: Extentions.h,v 1.1 2008/12/14 21:57:50 vpashka Exp $
// -------------------------------------------------------------------------
#ifndef Extentions_H_
#define Extentions_H_
// -------------------------------------------------------------------------
#include <string>
#include "UniXML.h"
#include "Debug.h"
#include "UniSetTypes.h"
#include "Calibration.h"
// -------------------------------------------------------------------------
namespace UniSetExtentions
{
	/*! Получение идентификатора объекта(процесса) разделяемой памяти */
	UniSetTypes::ObjectId getSharedMemoryID();

	/*! Получение времени для подтверждения "живости" */
	int getHeartBeatTime();

	xmlNode* findNode( xmlNode* node, const std::string snode, const std::string field );
	
	xmlNode* getCalibrationsSection();
	
	/*! замена служебных символов в строке 
	 * '\\' -> '\n'
	*/
	void escape_string( std::string& s );

	/*! Загрузка калибровочной диаграммы */
	Calibration* buildCalibrationDiagram( const std::string dname );

	extern DebugStream dlog;
}
// -------------------------------------------------------------------------
#endif // Extentions_H_
