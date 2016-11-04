// -----------------------------------------------------------------------------
#ifndef LostTestProc_H_
#define LostTestProc_H_
// -----------------------------------------------------------------------------
#include <unordered_map>
#include "Debug.h"
#include "LostTestProc_SK.h"
// -----------------------------------------------------------------------------
/* Цель: поймать расхождение значения в SM и в in_-переменной в процессе.
 * Тест: Каждые checkTime проверяем текущее значение в SM и в процессе, меняем в SM и опять проверяем.
 */
class LostTestProc:
	public LostTestProc_SK
{
	public:
		LostTestProc( UniSetTypes::ObjectId id, xmlNode* confnode = UniSetTypes::uniset_conf()->getNode("LostTestProc") );
		virtual ~LostTestProc();

	protected:
		LostTestProc();

		enum Timers
		{
			tmCheck
		};

		virtual void timerInfo( const UniSetTypes::TimerMessage* tm ) override;
		virtual void sysCommand( const UniSetTypes::SystemMessage* sm ) override;
		virtual void askSensors( UniversalIO::UIOCommand cmd ) override;
		virtual void sensorInfo( const UniSetTypes::SensorMessage* sm ) override;
		virtual std::string getMonitInfo() override;

		std::unordered_map<UniSetTypes::ObjectId,long> slist;
		size_t ncycle = { 0 };
		bool waitEmpty = { false };

	private:
};
// -----------------------------------------------------------------------------
#endif // LostTestProc_H_
// -----------------------------------------------------------------------------
