#ifndef ModbusRTUErrors_H_
#define ModbusRTUErrors_H_
// -------------------------------------------------------------------------
#include <string>
#include <iostream>
#include "Exceptions.h"
// -------------------------------------------------------------------------
namespace uniset
{
	// -------------------------------------------------------------------------
	namespace ModbusRTU
	{
		/*! Ошибки обмена
		    все ошибки > InternalErrorCode в сеть не посылаются...
		*/
		enum mbErrCode
		{
			erNoError               = 0,  /*!< нет ошибок */
			erUnExpectedPacketType  = 1,  /*!< Неожидаемый тип пакета (ошибка кода функции) */
			erBadDataAddress        = 2,  /*!< адрес запрещен к опросу или не существует */
			erBadDataValue          = 3,  /*!< недопустимое значение  */
			erHardwareError         = 4,  /*!< ошибка оборудования */
			erAnknowledge           = 5,  /*!< запрос принят в исполнению, но ещё не выполнен */
			erSlaveBusy             = 6,  /*!< контроллер занят длительной операцией (повторить запрос позже) */
			erOperationFailed       = 7,  /*!< запрашиваемая функция запрещена конфигурацией контроллера */
			erMemoryParityError     = 8,  /*!< ошибка паритета при чтении памяти */
			erGatewayUnavailable	= 10, /*!< шлюз не смог обработать запрос */
			erGatewayTargetUnavailable	= 11, /*!< устройство за шлюзом недоступно */

			// коды ошибок >= erInternalErrorCode не посылаются в ответах,
			// а используются только для внутренней диагностики
			erInternalErrorCode     = 100, /*!< коды ошибок используемые для внутренней работы */
			erInvalidFormat         = 111, /*!< неправильный формат */
			erBadCheckSum           = 112, /*!< У пакета не сошлась контрольная сумма */
			erBadReplyNodeAddress   = 113, /*!< Ответ на запрос адресован не мне или от станции, которую не спрашивали */
			erTimeOut               = 114, /*!< Тайм-аут при приеме ответа */
			erPacketTooLong         = 115, /*!< пакет длиннее буфера приема */
			erSessionClosed         = 116  /*!< соединение закрыто */
		};

		// ---------------------------------------------------------------------
		std::string mbErr2Str( mbErrCode e );
		// ---------------------------------------------------------------------
		class mbException:
			public uniset::Exception
		{
			public:
				mbException():
					uniset::Exception("mbException"), err(ModbusRTU::erNoError) {}
				mbException( ModbusRTU::mbErrCode err ):
					uniset::Exception(mbErr2Str(err)), err(err) {}


				ModbusRTU::mbErrCode err;

				friend std::ostream& operator<<(std::ostream& os, mbException& ex )
				{
					return os << "(" << ex.err << ") " << mbErr2Str(ex.err);
				}
		};
		// ---------------------------------------------------------------------
	} // end of namespace ModbusRTU
	// -------------------------------------------------------------------------
} // end of namespace uniset
// -------------------------------------------------------------------------
#endif // ModbusRTUErrors_H_
// -------------------------------------------------------------------------
