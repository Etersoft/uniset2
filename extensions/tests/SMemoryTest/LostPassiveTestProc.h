// -----------------------------------------------------------------------------
#ifndef LostPassiveTestProc_H_
#define LostPassiveTestProc_H_
// -----------------------------------------------------------------------------
#include <unordered_map>
#include "Debug.h"
#include "LostTestProc_SK.h"
// -----------------------------------------------------------------------------
/* Пассивный процесс, который только заказывает датчики, но не выставляет */
class LostPassiveTestProc:
	public LostTestProc_SK
{
	public:
		LostPassiveTestProc( UniSetTypes::ObjectId id, xmlNode* confnode = UniSetTypes::uniset_conf()->getNode("LostPassiveTestProc") );
		virtual ~LostPassiveTestProc();

		bool emptyQueue();
		long checkValue( UniSetTypes::ObjectId sid );

	protected:
		LostPassiveTestProc();

		virtual void askSensors( UniversalIO::UIOCommand cmd ) override;
		virtual void sensorInfo( const UniSetTypes::SensorMessage* sm ) override;

		std::mutex mut;
		std::unordered_map<UniSetTypes::ObjectId,long> slist;

	private:
};
// -----------------------------------------------------------------------------
#endif // LostPassiveTestProc_H_
// -----------------------------------------------------------------------------
