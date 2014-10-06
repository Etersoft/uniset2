#include <sstream>
#include <iostream>
#include <limits>
#include <algorithm>
#include "UniXML.h"
#include "Exceptions.h"
#include "Calibration.h"
#include "Extensions.h"
// ----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// ----------------------------------------------------------------------------
const long Calibration::outOfRange = std::numeric_limits<Calibration::TypeOfValue>::max();
// ----------------------------------------------------------------------------
Calibration::Part::Part():
k(0)
{

}

Calibration::Part::Part( const Point& pleft, const Point& pright ):
    p_left(pleft),
    p_right(pright),
    k(0)
{
    if( p_right.x < p_left.x )
    {
        Point t(p_right);
        p_right    = p_left;
        p_left    = t;
    }

    // вычисление коэффициента наклона (один раз в конструкторе)
    // k = (y2-y1)/(x2-x1)
    if( (p_right.y==0 && p_left.y==0) || (p_right.x==0 && p_left.x==0) )
        k = 0;
    else
        k = ( p_right.y - p_left.y ) / ( p_right.x - p_left.x );
}
// ----------------------------------------------------------------------------
bool Calibration::Part::check( const Point& p ) const
{
    return ( checkX(p.x) && checkY(p.y) );
}

bool Calibration::Part::checkX( const TypeOfValue& x ) const
{
    if( x < p_left.x || x > p_right.x )
        return false;
    return true;
}

bool Calibration::Part::checkY( const TypeOfValue& y ) const
{
    if( y < p_left.y || y > p_right.y )
        return false;

    return true;
}

// ----------------------------------------------------------------------------
Calibration::TypeOfValue Calibration::Part::getY( const TypeOfValue& x ) const
{
    if( !checkX(x) )
        return Calibration::outOfRange;

    if( x == left_x() )
        return left_y();

    if( x == right_x() )
        return right_y();

    return calcY(x);
}
// ----------------------------------------------------------------------------
Calibration::TypeOfValue Calibration::Part::getX( const TypeOfValue& y ) const
{
    if( !checkY(y) )
        return Calibration::outOfRange;

    if( y == left_y() )
        return left_x();

    if( y == right_y() )
        return right_x();

    return calcX(y);
}
// ----------------------------------------------------------------------------
Calibration::TypeOfValue Calibration::Part::calcY( const TypeOfValue& x ) const
{
    // y = y0 + kx
    return k*(x-p_left.x)+p_left.y;
}
// ----------------------------------------------------------------------------
Calibration::TypeOfValue Calibration::Part::calcX( const TypeOfValue& y ) const
{
    // x = (y - y0) / k
    if( k == 0 )
        return p_left.x;

    return ( y - p_left.y ) / k + p_left.x;
}
// ----------------------------------------------------------------------------

Calibration::Calibration():
minRaw(0),maxRaw(0),minVal(0),maxVal(0),rightVal(0),leftVal(0),rightRaw(0),leftRaw(0),
pvec(50),
myname(""),
szCache(5),
numCacheResort(20),
numCallToCache(5)
{
    cache.assign(szCache,CacheInfo());
}

// ----------------------------------------------------------------------------

Calibration::Calibration( const string& name, const string& confile ):
minRaw(0),maxRaw(0),minVal(0),maxVal(0),rightVal(0),leftVal(0),rightRaw(0),leftRaw(0),
pvec(50),
myname(name),
szCache(5),
numCacheResort(20),
numCallToCache(5)
{
    cache.assign(szCache,CacheInfo());
    build(name,confile,0);
}

// ----------------------------------------------------------------------------
Calibration::Calibration( xmlNode* node ):
minRaw(0),maxRaw(0),minVal(0),maxVal(0),rightVal(0),leftVal(0),rightRaw(0),leftRaw(0),pvec(100),
szCache(5),
numCacheResort(20),
numCallToCache(5)
{
    cache.assign(szCache,CacheInfo());
    UniXML_iterator it(node);
    myname = it.getProp("name");
    build("","",node);
}
// ----------------------------------------------------------------------------
Calibration::~Calibration()
{
}
// ----------------------------------------------------------------------------
void Calibration::build( const string& name, const string& confile, xmlNode* root )
{
    UniXML xml;
    try
    {
        if( root == 0 )
        {
            xml.open( confile );
            root = xml.findNode(xml.getFirstNode(),"diagram",name);
            if( !root )
            {
                ostringstream err;
                err << myname << "(Calibration::build): в файле " << confile
                    << " НЕ НАЙДЕНА диаграмма name=" << name;

                cerr << err.str() << endl;
                throw Exception(err.str());
            }
        }

        UniXML_iterator it(root);

        if( !it.goChildren() )
        {
            ostringstream err;
            err << myname << "(Calibration::build): в файле " << confile
                << " диаграмма name=" << name
                << " НЕ содержит элементов (point) " << endl;

            cerr << err.str() << endl;
            throw Exception(err.str());
        }

        bool prev = false;
        Point prev_point(0,0);
        unsigned int i=0;
        for(;it;it.goNext())
        {
            Point p(prev_point);
            p.x = atof(it.getProp("x").c_str());
            p.y = atof(it.getProp("y").c_str());

            if( p.x==0 || p.y==0 )
            {
                dwarn << myname << "(Calibration::build): (warn) x="
                        << p.x << " y=" << p.y << endl;
            }

            if( p.x > maxRaw )
                maxRaw = p.x;
            else if( p.x < minRaw )
                minRaw = p.x;

            if( p.y > maxVal )
                maxVal = p.y;
            else if( p.y < minVal )
                minVal = p.y;

            if( prev )
            {
                Part pt(prev_point,p);
                pvec[i++] = pt;
                if( i >= pvec.size() )
                   pvec.resize(pvec.size()+20);
            }
            else
                prev = true;

            prev_point = p;
        }

        pvec.resize(i); // приводим размер к фактическому..

        std::sort(pvec.begin(),pvec.end());

        auto beg = pvec.begin();
        auto end = pvec.end();

        if( pvec.size() > 0 )
        {
            leftRaw = beg->left_x();
            leftVal = beg->left_y();
            --end;
            rightRaw = end->right_x();
            rightVal = end->right_y();
        }
    }
    catch( Exception& ex )
    {
        dcrit << myname << "(Calibration::build): Failed open " << confile << endl;
        throw;
    }
}
// ----------------------------------------------------------------------------
// рекурсивная функция поиска методом "половинного деления"
static Calibration::PartsVec::iterator find_range( long raw, Calibration::PartsVec::iterator beg,
                                                              Calibration::PartsVec::iterator end )
{
    if( beg->checkX(raw) )
        return beg;

    if( end->checkX(raw) )
        return end;

    auto it = beg + std::distance(beg,end)/2;

    if( raw < it->left_x() )
        return find_range(raw,beg,it);

    if( raw > it->right_x() )
        return find_range(raw,it,end);

    return it;
}
// ----------------------------------------------------------------------------
long Calibration::getValue( long raw, bool crop_raw )
{
    // если x левее первого отрезка то берём первую точку...
    if( raw < leftRaw )
        return (crop_raw ? leftVal : outOfRange);

    // если x правее последнего то берём крайнюю точку...
    if( raw > rightRaw )
        return (crop_raw ? rightVal : outOfRange);

    if( szCache ) // > 0
    {
         for( auto &c: cache )
         {
              if( c.raw == raw )
              {
                  --numCallToCache;
                  c.cnt++;
                  if( numCallToCache )
                      return c.val;

                  long val = c.val; // после сортировки итератор станет недействительным, поэтому запоминаем..
                  sort(cache.begin(),cache.end());
                  numCallToCache = numCacheResort;
                  return val;
              }
         }
    }

    auto fit = find_range(raw, pvec.begin(), pvec.end());

    if( fit == pvec.end() )
    {
        if( szCache )
            insertToCache(raw,outOfRange);
        return outOfRange;
    }

    TypeOfValue q = fit->getY(raw);
    if( q != outOfRange )
    {
       if( szCache )
           insertToCache(raw, tRound(q) );
       return tRound(q);
    }

    if( szCache )
        insertToCache(raw,outOfRange);

    return outOfRange;
}
// ----------------------------------------------------------------------------
void Calibration::setCacheResortCycle( unsigned int n )
{
    numCacheResort = n;
    numCallToCache = n;
}
// ----------------------------------------------------------------------------
void Calibration::setCacheSize( unsigned int sz )
{
    sort(cache.begin(),cache.end()); // в порядке уменьшения обращений (см. CacheInfo::operator< )
    cache.resize(sz);
    szCache = sz;
}
// ----------------------------------------------------------------------------
void Calibration::insertToCache( const long raw, const long val )
{
    cache.pop_back(); // удаляем последний элемент (как самый неиспользуемый)
    cache.push_back( CacheInfo(raw,val) ); // добавляем в конец..
    sort(cache.begin(),cache.end()); // пересортируем в порядке уменьшения обращений (см. CacheInfo::operator< )
}
// ----------------------------------------------------------------------------
long Calibration::getRawValue( long cal, bool range )
{
    for( auto &it: pvec )
    {
        TypeOfValue q = it.getX(cal);
        if( q != outOfRange )
            return tRound(q);
    }

    if( range )
    {
        if( cal < leftVal )
            return leftRaw;

        if( cal > rightVal )
            return rightRaw;
    }

    return outOfRange;
}
// ----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, Calibration& c )
{
    os << "*******************" << endl;
    for( auto &it: c.pvec )
    {
        os << "[" << it.leftPoint().x << " : " << it.rightPoint().x << " ] --> ["
            << it.leftPoint().y  << " : " << it.rightPoint().y << " ]"
            << endl;
    }
    os << "*******************" << endl;
    return os;
}
// ----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, Calibration* c )
{
    os << "*******************" << endl;
    for( auto &it: c->pvec )
    {
        os << "[" << it.leftPoint().x << " : " << it.rightPoint().x << " ] --> ["
            << it.leftPoint().y  << " : " << it.rightPoint().y << " ]"
            << endl;
    }
    os << "*******************" << endl;

    return os;
}
// ----------------------------------------------------------------------------
