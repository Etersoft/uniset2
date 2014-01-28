#ifndef TDelay_H_
#define TDelay_H_
// --------------------------------------------------------------------------
#include "PassiveTimer.h"
#include "Element.h"
// ---------------------------------------------------------------------------
// add on delay, off delay

class TDelay:
	public Element
{

	public:
		TDelay( Element::ElementID id, int delayMS=0, int inCount=0 );
	    virtual ~TDelay();


		virtual void tick();
		virtual void setIn( int num, bool state );
		virtual bool getOut();
		virtual std::string getType(){ return "Delay"; }

		virtual void setDelay(int timeMS);
		inline int getDelay(){ return delay; }

	protected:
		TDelay():myout(false),delay(0){};

		bool myout;
		PassiveTimer pt;
		int delay;

	private:
};
// ---------------------------------------------------------------------------
#endif
// ---------------------------------------------------------------------------

