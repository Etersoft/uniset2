#ifndef TDelay_H_
#define TDelay_H_
// --------------------------------------------------------------------------
#include "PassiveTimer.h"
#include "Element.h"
// ---------------------------------------------------------------------------
// "ON" delay element
// Сбрасывается без задержки.. а срабатывает с задержкой.
class TDelay:
	public Element
{

	public:
		TDelay( Element::ElementID id, timeout_t delayMS = 0, unsigned int inCount = 0 );
		virtual ~TDelay();


		virtual void tick() override;
		virtual void setIn( int num, bool state ) override;
		virtual bool getOut() override;
		virtual std::string getType() override
		{
			return "Delay";
		}

		void setDelay( timeout_t timeMS );
		inline timeout_t getDelay()
		{
			return delay;
		}

	protected:
		TDelay(): myout(false), delay(0) {};

		bool myout;
		PassiveTimer pt;
		timeout_t delay;

	private:
};
// ---------------------------------------------------------------------------
#endif
// ---------------------------------------------------------------------------

