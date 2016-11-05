// -----------------------------------------------------------------------------
#ifndef LostTestProc_H_
#define LostTestProc_H_
// -----------------------------------------------------------------------------
#include <unordered_map>
#include "Debug.h"
#include "LostPassiveTestProc.h"
// -----------------------------------------------------------------------------
/* Цель: поймать расхождение значения в SM и в in_-переменной в процессе.
 * Тест: Каждые checkTime проверяем текущее значение в SM и в процессе, меняем в SM и опять проверяем.
 *
 * Заодно если инициализирован child то проверяем что у него тоже все входы совпадают со значениями в SM.
 */
class LostTestProc:
	public LostPassiveTestProc
{
	public:
		LostTestProc( UniSetTypes::ObjectId id, xmlNode* confnode = UniSetTypes::uniset_conf()->getNode("LostTestProc") );
		virtual ~LostTestProc();

		void setChildPassiveProc( const std::shared_ptr<LostPassiveTestProc>& lp );

	protected:
		LostTestProc();

		enum Timers
		{
			tmCheck
		};

		virtual void timerInfo( const UniSetTypes::TimerMessage* tm ) override;
		virtual void sysCommand( const UniSetTypes::SystemMessage* sm ) override;
		virtual std::string getMonitInfo() override;

		size_t ncycle = { 0 };
		bool waitEmpty = { false };

		std::shared_ptr<LostPassiveTestProc> child;

	private:
};
// -----------------------------------------------------------------------------
#endif // LostTestProc_H_
// -----------------------------------------------------------------------------
