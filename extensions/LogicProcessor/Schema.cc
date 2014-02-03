#include <sstream>
#include <iostream>
#include "Extensions.h"
#include "Schema.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace UniSetExtensions;
// -------------------------------------------------------------------------

Schema::Schema()
{
}

Schema::~Schema()
{
    for( auto &it: emap )
    {
        if( it.second != 0 )
        {
            delete it.second;
            it.second = 0;
        }
    }
}
// -------------------------------------------------------------------------
void Schema::link( Element::ElementID rootID, Element::ElementID childID, int numIn )
{
    Element* e1 = 0;
    Element* e2 = 0;

    auto it = emap.find(rootID);
    if( it == emap.end() )
    {
        ostringstream msg;
        msg << "Schema: элемент id=" << rootID << " NOT FOUND!";
        throw LogicException(msg.str());
    }
    e1 = it->second;
    
    it = emap.find(childID);
    if( it == emap.end() )
    {
        ostringstream msg;
        msg << "Schema: элемент id=" << childID << " NOT FOUND!";
        throw LogicException(msg.str());
    }
    e2 = it->second;

    e1->addChildOut(e2, numIn);
    
    // сохраняем в список соединений
    inLinks.push_front(INLink(e1,e2,numIn));
}
// -------------------------------------------------------------------------
void Schema::unlink( Element::ElementID rootID, Element::ElementID childID )
{
    Element* e1(0);
    Element* e2(0);

    auto it = emap.find(rootID);
    if( it == emap.end() )
    {
        ostringstream msg;
        msg << "Schema: элемент id=" << rootID << " NOT FOUND!";
        throw LogicException(msg.str());
    }
    e1 = it->second;

    it = emap.find( childID );
    if( it == emap.end() )
    {
        ostringstream msg;
        msg << "Schema: element id=" << childID << " NOT FOUND!";
        throw LogicException(msg.str());
    }
    e2 = it->second;

    e1->delChildOut(e2);

    // удаляем из списка соединений
    for( auto lit=inLinks.begin(); lit!=inLinks.end(); ++lit )
    {
        if( lit->from == e1 && lit->to == e2 )
        {
            inLinks.erase(lit);
            break;
        }
    }
}
// -------------------------------------------------------------------------
void Schema::extlink( const string& name, Element::ElementID childID, int numIn )
{
    auto it = emap.find(childID);
    if( it == emap.end() )
    {
        ostringstream msg;
        msg << "Schema: element id=" << childID << " NOT FOUND!";
        throw LogicException(msg.str());
    }

    Element* el(it->second);

//     добавляем новое соединение    
//    el->addInput(numIn);
//    уже должен быть

    // заносим в список
    extLinks.push_front( EXTLink(name,el,numIn) );
}
// -------------------------------------------------------------------------
Element* Schema::manage( Element* el )
{
    dinfo << "Schema: manage new element id=" << el->getId()
             << " type=" << el->getType()
             << " inputs=" << el->inCount() << endl;

    emap[el->getId()] = el;
    return el;
}
// -------------------------------------------------------------------------
void Schema::remove( Element* el )
{
    for( auto it=emap.begin(); it!=emap.end(); ++it )
    {
        if( it->second != el )
        {
            emap.erase(it);
            return;
        }
    }

    // помечаем внутренние связи
    for( auto &lit: inLinks )
    {
        if( lit.from == el )
            lit.from = 0;
            
        if( lit.to == el )
            lit.to = 0;
    }

    // помечаем внешние связи
    for( auto &lit: extLinks )
    {
        if( lit.to == el )
            lit.to = 0;
    }

}
// -------------------------------------------------------------------------
void Schema::setIn( Element::ElementID ID, int inNum, bool state )
{
    auto it = emap.find(ID);
    if( it != emap.end() )
        it->second->setIn(inNum,state);
}
// -------------------------------------------------------------------------
bool Schema::getOut( Element::ElementID ID )
{
    auto it = emap.find(ID);
    if( it != emap.end() )
        return it->second->getOut();

    ostringstream msg;
    msg << "Schema: element id=" <<ID << " NOT FOUND!";
    throw LogicException(msg.str());
}
// -------------------------------------------------------------------------
Element* Schema::find( Element::ElementID id )
{
    auto it = emap.find(id);
    if( it != emap.end() )
        return it->second;
    return 0;
}
// -------------------------------------------------------------------------
Element* Schema::findExtLink( const string& name )
{
    // помечаем внешние связи
    for( auto &it: extLinks )
    {
        if( it.name == name )
            return it.to;        
    }
    
    return 0;
}
// -------------------------------------------------------------------------
