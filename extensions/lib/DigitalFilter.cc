// --------------------------------------------------------------------------
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <iostream>
#include "UniSetTypes.h"
#include "DigitalFilter.h"
//--------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
//--------------------------------------------------------------------------
DigitalFilter::DigitalFilter( unsigned int bufsize, double T, double lsq,
                              int iir_thr, double iir_coeff_prev,
                              double iir_coeff_new ):
    Ti(T),
    val(0),
    M(0),
    S(0),
    tmr(UniSetTimer::WaitUpTime),
    maxsize(bufsize),
    mvec(bufsize),
    w(bufsize),
    lsparam(lsq),
    ls(0),
    thr(iir_thr),
    prev(0),
    coeff_prev(iir_coeff_prev),
    coeff_new(iir_coeff_new)
{
}
//--------------------------------------------------------------------------
DigitalFilter::~DigitalFilter()
{
}
//--------------------------------------------------------------------------
void DigitalFilter::setSettings( unsigned int bufsize, double T, double lsq,
                                 int iir_thr, double iir_coeff_prev,
                                 double iir_coeff_new )
{
    Ti = T;
    maxsize = bufsize;
    if( maxsize < 1 )
        maxsize = 1;

    coeff_prev = iir_coeff_prev;
    coeff_new = iir_coeff_new;
    if( iir_thr > 0 )
        thr = iir_thr;

    if( buf.size() > maxsize )
    {
        // удаляем лишние (первые) элементы
        int sub = buf.size() - maxsize;
        for( int i=0; i<sub; i++ )
            buf.erase( buf.begin() );
    }

    if( w.size() != maxsize || lsq != lsparam )
        w.assign(maxsize, 1.0/maxsize);
    lsparam = lsq;

    mvec.resize(maxsize);
}
//--------------------------------------------------------------------------
void DigitalFilter::init( int val )
{
    buf.clear();
    for( unsigned int i=0; i<maxsize; i++ )
        buf.push_back(val);

    w.assign(maxsize, 1.0/maxsize);

    tmr.reset();
    this->val = val;
}
//--------------------------------------------------------------------------
double DigitalFilter::firstLevel()
{
    // считаем среднее арифметическое
    M=0;
    for( FIFOBuffer::iterator i=buf.begin(); i!=buf.end(); ++i )
        M = M + (*i);

    M = M/buf.size();

    // считаем среднеквадратичное отклонение
    S=0;
    double r=0;
    for( FIFOBuffer::iterator i=buf.begin(); i!=buf.end(); ++i )
    {
        r = M-(*i);
        S = S + r*r;
    }

    S = S/buf.size();
    S = sqrt(S);

    if( S == 0 )
        return M;

    // Находим среднее арифметическое без учета элементов, отклонение которых вдвое превышает среднеквадратичное
    int n = 0;
    double val = 0; // Конечное среднее значение
    for( FIFOBuffer::iterator i=buf.begin(); i!=buf.end(); ++i )
    {
        if( fabs(M-(*i)) > S*2 )
        {
            val = val + (*i);
            n = n + 1;
        }
    }

    if( n==0 )
        return M;

    return ( val / n );
}
//--------------------------------------------------------------------------
int DigitalFilter::filterRC( int rawval )
{
    if( Ti <= 0 )
        return rawval;

    return lroundf(secondLevel(rawval));
}
//--------------------------------------------------------------------------

double DigitalFilter::secondLevel( double rawval )
{
    if( Ti <= 0 )
        return rawval;

    // Измеряем время с прошлого вызова функции
    int dt = tmr.getCurrent();
    if( dt == 0 )
        return val;

    tmr.reset();

    // Сама формула RC фильтра
    val = (rawval + Ti*val/dt) / (Ti/dt + 1);

    return val;
}
//--------------------------------------------------------------------------
int DigitalFilter::filter1( int newval )
{
    if( maxsize < 1 )
        return newval;

    add(newval);
    return lroundf(firstLevel());
}
//--------------------------------------------------------------------------
void DigitalFilter::add( int newval )
{
    // помещаем очередное значение в буфер
    // удаляя при этом старое (FIFO)
    buf.push_back(newval);
    if( buf.size() > maxsize )
        buf.erase( buf.begin() );
}
//--------------------------------------------------------------------------
int DigitalFilter::current1()
{
    return lroundf(firstLevel());
}
//--------------------------------------------------------------------------
int DigitalFilter::currentRC()
{
    return lroundf(secondLevel(current1()));
}
//--------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const DigitalFilter& d )
{
    os << "(" << d.buf.size() << ")[";
    for( DigitalFilter::FIFOBuffer::const_iterator i=d.buf.begin(); i!=d.buf.end(); ++i )
    {
        os << " " << setw(5) << (*i);
    }
    os << " ]";
    return os;
}
//--------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const DigitalFilter* d)
{
    return os << (*d);
}
//--------------------------------------------------------------------------
int DigitalFilter::median( int newval )
{
    if( maxsize < 1 )
        return newval;

    add(newval);

//    FIFOBuffer::iterator it = buf.begin();
//    for( unsigned int i=0; i<maxsize && it!=buf.end(); i++,it++ )
//        mvec[i] = (*it);

//    copy(buf.begin(),buf.end(),mvec.begin());

    mvec.assign(buf.begin(),buf.end());

    sort(mvec.begin(),mvec.end());

    return mvec[maxsize/2];
}
//--------------------------------------------------------------------------
int DigitalFilter::currentMedian()
{
    return mvec[maxsize/2];
}
//--------------------------------------------------------------------------
int DigitalFilter::leastsqr( int newval )
{
    ls = 0;

    add(newval);

    // Цифровая фильтрация
    FIFOBuffer::const_iterator it = buf.begin();
    for( unsigned int i=0; i<maxsize; i++,it++ )
        ls += *it * w[i];

    // Вычисляем ошибку выхода
    double er = newval - ls;

    // Обновляем коэффициенты
    double u = 2 * (lsparam/maxsize) * er;
    it = buf.begin();
    for( unsigned int i=0; i<maxsize; i++,it++ )
        w[i] = w[i] + *it * u;

    return lroundf(ls);
}
//--------------------------------------------------------------------------
int DigitalFilter::currentLS()
{
    return lroundf(ls);
}
//--------------------------------------------------------------------------
int DigitalFilter::filterIIR( int newval )
{
    if( newval > prev + thr || newval < prev - thr || maxsize < 1 )
    {
        if( maxsize > 0 )
            init(newval);
        prev = newval;
    }
    else
    {
        double aver=0;

        add(newval);
        for( FIFOBuffer::iterator i = buf.begin(); i != buf.end(); ++i )
            aver += *i;
        aver /= maxsize;
        prev = lroundf((coeff_prev * prev + coeff_new * aver)/(coeff_prev + coeff_new));
    }
    return prev;
}
//--------------------------------------------------------------------------
int DigitalFilter::currentIIR()
{
    return prev;
}
//--------------------------------------------------------------------------
