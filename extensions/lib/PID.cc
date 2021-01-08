/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// -------------------------------------------------------------------------
#include <iomanip>
#include "PID.h"
// -----------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    PID::PID():
        Y(0), Kc(0),
        Ti(0), Td(0),
        vlim(2.0),
        d0(0),
        d1(0),
        d2(0),
        prevTs(0)
    {
        reset();
    }

    // -----------------------------------------------------------------------------

    PID::~PID()
    {

    }

    // -----------------------------------------------------------------------------
    void PID::step( const double& X, const double& Z, const double& Ts )
    {

        // Чтобы не пересчитывать коэффициенты на каждом шаге
        // сделан пересчёт только по изменению
        //    d0         = 1+Ts/Ti+Td/Ts;
        //    d1         = -1-2*Td/Ts;
        //    d2         = Td/Ts;

        //    в случае изменения Td и Ts за вызов recalc отвечает "Родитель"(PIDControl)
        if( prevTs != Ts )
        {
            prevTs = Ts;
            recalc();
        }

        sub2     = sub1;// ошибка 2 шага назад
        sub1     = sub; // ошибка 1 шаг назад
        sub     = Z - X; // текущая ошибка
        // NOTE: в первоисточнике было "текущее"(X) - "заданное"(Z),
        //    но правильно именно Z-X (проверено!)

        // окончальное выходное(расчётное) значение
        Y = Y + Kc * ( d0 * sub + d1 * sub1 + d2 * sub2 );

        if( Y > vlim ) Y = vlim;
        else if ( Y < -vlim ) Y = -vlim;
    }
    // -----------------------------------------------------------------------------
    void PID::reset()
    {
        sub2 = sub1 = sub = Y = 0;
    }
    // -----------------------------------------------------------------------------
    std::ostream& operator<<( std::ostream& os, PID& p )
    {
        return os << "Kc=" << std::setw(4) << p.Kc
               << "  Ti=" << std::setw(4) << p.Ti
               << "  Td=" << std::setw(4) << p.Td
               << "  Y=" << std::setw(4) << p.Y
               << "  vlim=" << std::setw(4) << p.vlim
               << "  sub2=" << setw(4) << p.sub2
               << "  sub1=" << setw(4) << p.sub1
               << "  sub=" << setw(4) << p.sub;
    }
    // --------------------------------------------------------------------------

    void PID::recalc()
    {
        //    d0         = 1+prevTs/Ti+Td/prevTs;
        //    d1         = -1-2*Td/prevTs;
        d2         = Td / prevTs;
        d1         = -1 - 2 * d2;
        d0         = 1 + prevTs / Ti + d2;
    }
    // --------------------------------------------------------------------------
} // end of namespace uniset
