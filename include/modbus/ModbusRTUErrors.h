#ifndef ModbusRTUErrors_H_
#define ModbusRTUErrors_H_
// -------------------------------------------------------------------------
#include <string>
#include <iostream>
#include "Exceptions.h"
// -------------------------------------------------------------------------
namespace ModbusRTU
{
	/*! Ошибки обмена 
		все ошибки > InternalErrorCode в сеть не посылаются...
	*/
	enum mbErrCode
	{
		erNoError 				= 0, /*!< нет ошибок */
	    erUnExpectedPacketType  = 1, /*!< Неожидаемый тип пакета (ошибка кода функции) */
		erBadDataAddress 		= 2, /*!< адрес запрещен к опросу или не существует */
		erBadDataValue 			= 3, /*!< недопустимое значение  */
	    erHardwareError 		= 4, /*!< ошибка оборудования */
		erAnknowledge			= 5, /*!< запрос принят в исполнению, но ещё не выполнен */
		erSlaveBusy 			= 6, /*!< контроллер занят длительной операцией (повторить запрос позже) */
		erOperationFailed		= 7, /*!< запрашиваемая функция запрещена конфигурацией контроллера */
		erMemoryParityError		= 8, /*!< ошибка паритета при чтении памяти */

		erInternalErrorCode	= 10,	/*!< коды ошибок используемые для внутренней работы */
		erInvalidFormat 	= 11, 	/*!< неправильный формат */
	    erBadCheckSum 		= 12, 	/*!< У пакета не сошлась контрольная сумма */
		erBadReplyNodeAddress = 13, /*!< Ответ на запрос адресован не мне или от станции,которую не спрашивали */
	    erTimeOut 			= 14,	/*!< Тайм-аут при приеме ответа */
    	erPacketTooLong 	= 15 	/*!< пакет длинее буфера приема */
	};

	// ---------------------------------------------------------------------
	std::string mbErr2Str( mbErrCode e );
	// ---------------------------------------------------------------------
	class mbException:
		UniSetTypes::Exception
	{
		public:
			mbException():
				UniSetTypes::Exception("mbException"),err(ModbusRTU::erNoError){}
			mbException( ModbusRTU::mbErrCode err ):
				UniSetTypes::Exception(mbErr2Str(err)),err(err){}
			
			
			ModbusRTU::mbErrCode err;

			friend std::ostream& operator<<(std::ostream& os, mbException& ex )
			{
				return os << "(" << ex.err << ") " << mbErr2Str(ex.err);
			}
	};
	// ---------------------------------------------------------------------
}
// -------------------------------------------------------------------------
#endif // ModbusRTUErrors_H_
// -------------------------------------------------------------------------
