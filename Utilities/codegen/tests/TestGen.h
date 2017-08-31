// -----------------------------------------------------------------------------
#ifndef TestGen_H_
#define TestGen_H_
// -----------------------------------------------------------------------------
#include "TestGen_SK.h"
// -----------------------------------------------------------------------------
class TestGen:
	public TestGen_SK
{
	public:
		TestGen( uniset::ObjectId id, xmlNode* confnode = uniset::uniset_conf()->getNode("TestGen") );
		virtual ~TestGen();

	protected:
		TestGen();

		virtual void step() override;
		virtual void sensorInfo( const uniset::SensorMessage* sm ) override;
		virtual void timerInfo( const uniset::TimerMessage* tm ) override;
		virtual void sysCommand( const uniset::SystemMessage* sm ) override;
#ifndef DISABLE_REST_API
		virtual void httpGetUserData( Poco::JSON::Object::Ptr& jdata ) override;
#endif
	private:
		bool bool_var = { false };
		int int_var = {0};
		uniset::timeout_t t_val = { 0 };
};
// -----------------------------------------------------------------------------
#endif // TestGen_H_
// -----------------------------------------------------------------------------
