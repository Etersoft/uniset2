// -----------------------------------------------------------------------------
#ifndef TestProc_H_
#define TestProc_H_
// -----------------------------------------------------------------------------
#include <vector>
#include "Debug.h"
#include "TestProc_SK.h"
#include "modbus/ModbusTCPServerSlot.h"
// -----------------------------------------------------------------------------
class TestProc:
	public TestProc_SK
{
	public:
		TestProc( UniSetTypes::ObjectId id, xmlNode* confnode = UniSetTypes::uniset_conf()->getNode("TestProc") );
		virtual ~TestProc();

	protected:
		TestProc();

		enum Timers
		{
			tmChange,
			tmCheckWorking,
			tmCheck,
			tmLogControl
		};

		virtual void step();
		virtual void sensorInfo( const UniSetTypes::SensorMessage* sm );
		virtual void timerInfo( const UniSetTypes::TimerMessage* tm );
		virtual void sysCommand( const UniSetTypes::SystemMessage* sm );

		void test_depend();
		void test_undefined_state();
		void test_thresholds();
		void test_loglevel();

	private:
		bool state;
		bool undef;

		std::vector<Debug::type> loglevels;
		std::vector<Debug::type>::iterator lit;

		std::shared_ptr<ModbusTCPServerSlot> mbslave;
		/*! обработка 0x06 */
		ModbusRTU::mbErrCode writeOutputSingleRegister( ModbusRTU::WriteSingleOutputMessage& query,
				ModbusRTU::WriteSingleOutputRetMessage& reply );

		std::shared_ptr< ThreadCreator<TestProc> > mbthr;
		void mbThread();
};
// -----------------------------------------------------------------------------
#endif // TestProc_H_
// -----------------------------------------------------------------------------
