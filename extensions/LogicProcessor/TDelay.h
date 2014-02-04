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


        virtual void tick() override;
        virtual void setIn( int num, bool state ) override;
        virtual bool getOut() override;
        virtual std::string getType() override { return "Delay"; }

        void setDelay( int timeMS );
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

