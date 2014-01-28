// --------------------------------------------------------------------------
#ifndef Pulse_H_
#define Pulse_H_
// --------------------------------------------------------------------------
#include <iostream>
#include "PassiveTimer.h"
// --------------------------------------------------------------------------
/*! Класс реализующий формирование импульсов заданной длительности(t1) и заданных пауз между ними(t0).
    Класс пассивный, для работы требует постоянного вызова функции step().
    Для получения текущего состояния "выхода" использовать out().
    Формирование импульсов включается функцией run() либо функцией set(true).
    Вызов reset() тоже включает формирование импульсов.
    Выключается формирование вызовом set(false).
*/
class Pulse
{
    public:
        Pulse(): ostate(false),isOn(false),
            t1_msec(0),t0_msec(0)
        {}
        ~Pulse(){}

        // t1_msec - интервал "вкл"
        // t0_msec - интерфал "откл"
        inline void run( int _t1_msec, int _t0_msec )
        {
            t1_msec = _t1_msec;
            t0_msec = _t0_msec;
            t1.setTiming(t1_msec);
            t0.setTiming(t0_msec);
            set(true);
        }

        inline void set_next( int _t1_msec, int _t0_msec )
        {
            t1_msec    = _t1_msec;
            t0_msec = _t0_msec;
        }

        inline void reset()
        {
            set(true);
        }

        inline bool step()
        {
            if( !isOn )
            {
                ostate = false;
                return false;
            }

            if( ostate && t1.checkTime() )
            {
                ostate = false;
                t0.setTiming(t0_msec);
            }
            else if( !ostate && t0.checkTime() )
            {
                t1.setTiming(t1_msec);
                ostate = true;
            }

            return ostate;
        }

        inline bool out(){ return ostate; }

        inline void set( bool state )
        {
            isOn = state;
            if( !isOn )
                ostate = false;
            else
            {
                t1.reset();
                t0.reset();
                ostate     = true;
            }
        }

        friend std::ostream& operator<<(std::ostream& os, Pulse& p )
        {
            return os     << " idOn=" << p.isOn
                        << " t1=" << p.t1.getInterval()
                        << " t0=" << p.t0.getInterval()
                        << " out=" << p.ostate;
        }

        friend std::ostream& operator<<(std::ostream& os, Pulse* p )
        {
            return os << (*p);
        }


    protected:
        PassiveTimer t1;    // таймер "1"
        PassiveTimer t0;    // таймер "0"
        bool ostate;
        bool isOn;
        long t1_msec;
        long t0_msec;

};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
