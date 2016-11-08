// --------------------------------------------------------------------------
#ifndef NullSM_H_
#define NullSM_H_
// --------------------------------------------------------------------------
#include <string>
#include "IONotifyController.h"
#include "NCRestorer.h"
// --------------------------------------------------------------------------
class NullSM:
	public uniset::IONotifyController
{
	public:
		NullSM( uniset::ObjectId id, const std::string& datfile );

		virtual ~NullSM();

	protected:

		virtual void logging( uniset::SensorMessage& sm ) override {};

		virtual void dumpOrdersList( const uniset::ObjectId sid, const IONotifyController::ConsumerListInfo& lst ) override {};
		virtual void dumpThresholdList( const uniset::ObjectId sid, const IONotifyController::ThresholdExtList& lst ) override {};

	private:

};
// --------------------------------------------------------------------------
#endif // NullSM_H_
// --------------------------------------------------------------------------
