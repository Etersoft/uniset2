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
        TestProc( uniset::ObjectId id, xmlNode* confnode = uniset::uniset_conf()->getNode("TestProc") );
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
        virtual void sensorInfo( const uniset::SensorMessage* sm );
        virtual void timerInfo( const uniset::TimerMessage* tm );
        virtual void sysCommand( const uniset::SystemMessage* sm );

        void test_depend();
        void test_undefined_state();
        void test_thresholds();
        void test_loglevel();

    private:
        bool state = { false };
        bool undef = { false };

        std::vector<Debug::type> loglevels;
        std::vector<Debug::type>::iterator lit;

        std::shared_ptr<uniset::ModbusTCPServerSlot> mbslave;
        /*! обработка 0x06 */
        uniset::ModbusRTU::mbErrCode writeOutputSingleRegister( uniset::ModbusRTU::WriteSingleOutputMessage& query,
                uniset::ModbusRTU::WriteSingleOutputRetMessage& reply );

        std::shared_ptr< uniset::ThreadCreator<TestProc> > mbthr;
        void mbThread();
};
// -----------------------------------------------------------------------------
#endif // TestProc_H_
// -----------------------------------------------------------------------------
