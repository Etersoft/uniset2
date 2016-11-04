// -----------------------------------------------------------------------------
#ifndef LostTestProc_H_
#define LostTestProc_H_
// -----------------------------------------------------------------------------
#include <vector>
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

	private:
};
// -----------------------------------------------------------------------------
#endif // LostTestProc_H_
// -----------------------------------------------------------------------------
