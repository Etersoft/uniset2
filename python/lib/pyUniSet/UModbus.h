#ifndef UModbus_H_
#define UModbus_H_
// --------------------------------------------------------------------------
#include "Configuration.h"
#include "UInterface.h"
#include "modbus/ModbusTCPMaster.h"
#include "modbus/ModbusTypes.h"
#include "extensions/VTypes.h"
#include "Debug.h"
#include "UTypes.h"
#include "UExceptions.h"
// --------------------------------------------------------------------------
class UModbus
{
	public:

		UModbus();
		~UModbus();

		inline std::string getUIType()
		{
			return string("modbus");
		}

		inline bool isWriteFunction( int mbfunc )
		{
			return ModbusRTU::isWriteFunction((ModbusRTU::SlaveFunctionCode)mbfunc);
		}

		// выставление паметров связи, без установления соединения (!)
		void prepare( const std::string& ip, int port )throw(UException);

		void connect( const std::string& ip, int port )throw(UException);
		inline int conn_port()
		{
			return port;
		}
		inline std::string conn_ip()
		{
			return ip;
		}
		inline bool isConnection()
		{
			return (mb && mb->isConnection());
		}

		inline void setTimeout( int msec )
		{
			tout_msec = msec;
		}

		/*! Универсальная функция для чтения регистров.
		 * Если не указывать ip и порт, будут использованы, те
		 * чтобы были заданы в UModbus::connect(). Если заданы другие ip и port,
		 * будет сделано переподключение..
		 */
		long mbread( int addr, int mbreg, int mbfunc,
					 const std::string& vtype, int nbit = -1,
					 const std::string& ip = "", int port = -1 )throw(UException);

		long getWord( int addr, int mbreg, int mbfunc = 0x4 )throw(UException);
		long getByte( int addr, int mbreg, int mbfunc = 0x4 )throw(UException);
		bool getBit( int addr, int mbreg, int mbfunc = 0x2 )throw(UException);

		/*! Функция записи регистров 0x06 или 0x10 задаётся параметром \a mbfunc.
		 * Если не указывать ip и порт, будут использованы, те
		 * чтобы были заданы в UModbus::connect(). Если заданы другие ip и port,
		 * будет сделана переподключение..
		*/
		void mbwrite( int addr, int mbreg, int val, int mbfunc, const std::string& ip = "", int port = -1 )throw(UException);

	protected:
		long data2value( VTypes::VType vt, ModbusRTU::ModbusData* data );

	private:
		// DebugStream dlog;
		ModbusTCPMaster* mb;
		int port;
		string ip;
		int tout_msec;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
