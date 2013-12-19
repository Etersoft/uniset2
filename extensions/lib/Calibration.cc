#include <sstream>
#include <iostream>
#include "UniXML.h"
#include "Exceptions.h"
#include "Calibration.h"
// ----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// ----------------------------------------------------------------------------

Calibration::Part::Part( Point& pleft, Point& pright ):
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
bool Calibration::Part::check( Point& p )
{
    return ( checkX(p.x) && checkY(p.y) );
}

bool Calibration::Part::checkX( TypeOfValue x )
{
    if( x < p_left.x || x > p_right.x )
        return false;
    return true;
}

bool Calibration::Part::checkY( TypeOfValue y )
{
    if( y < p_left.y || y > p_right.y )
        return false;

    return true;
}

// ----------------------------------------------------------------------------
Calibration::TypeOfValue Calibration::Part::getY( TypeOfValue x )
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
Calibration::TypeOfValue Calibration::Part::getX( TypeOfValue y )
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
Calibration::TypeOfValue Calibration::Part::calcY( TypeOfValue x )
{
    // y = y0 + kx
    return k*(x-p_left.x)+p_left.y;
}
// ----------------------------------------------------------------------------
Calibration::TypeOfValue Calibration::Part::calcX( TypeOfValue y )
{
    // x = (y - y0) / k
    if( k == 0 )
        return p_left.x;

    return ( y - p_left.y ) / k + p_left.x;
}
// ----------------------------------------------------------------------------

Calibration::Calibration():
minRaw(0),maxRaw(0),minVal(0),maxVal(0),rightVal(0),leftVal(0),rightRaw(0),leftRaw(0),
myname("")
{
}

// ----------------------------------------------------------------------------

Calibration::Calibration( const string& name, const string& confile ):
minRaw(0),maxRaw(0),minVal(0),maxVal(0),rightVal(0),leftVal(0),rightRaw(0),leftRaw(0),
myname(name)
{
    build(name,confile,0);
}

// ----------------------------------------------------------------------------
Calibration::Calibration( xmlNode* node ):
minRaw(0),maxRaw(0),minVal(0),maxVal(0),rightVal(0),leftVal(0),rightRaw(0),leftRaw(0)
{
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
        for(;it;it.goNext())
        {
            Point p(prev_point);
            p.x = atof(it.getProp("x").c_str());
            p.y = atof(it.getProp("y").c_str());
            
            if( p.x==0 || p.y==0 )
            {
                cerr << myname << "(Calibration::build): (warn) x=" 
                        << p.x << " y=" << p.y << endl;
            }

//            cout << myname << "(Calibration::build):"
//                        << "\tadd x=" << p.x << " y=" << p.y << endl;

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
//                cout << myname << "(Calibration::build):"
//                        << "\tadd x=" << p.x << " y=" << p.y
//                        << " prev.x=" << prev_point.x
//                        << " prev.y=" << prev_point.y
//                        << endl;
                Part pt(prev_point,p);
                plist.push_back(pt);
            }
            else
                prev = true;

            prev_point = p;
        }
        
        plist.sort();
        
        PartsList::iterator beg = plist.begin();
        PartsList::iterator end = plist.end();
        
        if( plist.size() > 0 )
        {
            leftRaw = beg->left_x();
            leftVal = beg->left_y();
            end--;
            rightRaw = end->right_x();
            rightVal = end->right_y();
        }
    }
    catch( Exception& ex )
    {
        cerr << myname << "(Calibration::build): Failed open " << confile << endl;
        throw;
    }
}
// ----------------------------------------------------------------------------
long Calibration::getValue( long raw, bool crop_raw )
{
    if( crop_raw )
    {
        // если x левее первого отрезка то берём первую точку...
        if( raw < leftRaw )
            return leftVal;
        
        // если x правее последнего то берём крайнюю точку...
        if( raw > rightRaw )
            return rightVal;
    }
    
    for( PartsList::iterator it=plist.begin(); it!=plist.end(); ++it )
    {
        TypeOfValue q = it->getY(raw);
        if( q != outOfRange )
            return tRound(q);
    }

    return outOfRange;
}
// ----------------------------------------------------------------------------
long Calibration::getRawValue( long cal, bool range )
{
    for( PartsList::iterator it=plist.begin(); it!=plist.end(); ++it )
    {
        TypeOfValue q = it->getX(cal);
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
    for( Calibration::PartsList::iterator it=c.plist.begin(); it!=c.plist.end(); ++it )
    {
        os << "[" << it->leftPoint().x << " : " << it->rightPoint().x << " ] --> [" 
            << it->leftPoint().y  << " : " << it->rightPoint().y << " ]"
            << endl;
    }
    os << "*******************" << endl;
    return os;
}
// ----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, Calibration* c )
{
    os << "*******************" << endl;
    for( Calibration::PartsList::iterator it=c->plist.begin(); it!=c->plist.end(); ++it )
    {
        os << "[" << it->leftPoint().x << " : " << it->rightPoint().x << " ] --> [" 
            << it->leftPoint().y  << " : " << it->rightPoint().y << " ]"
            << endl;
    }
    os << "*******************" << endl;
    
    return os;
}
// ----------------------------------------------------------------------------
