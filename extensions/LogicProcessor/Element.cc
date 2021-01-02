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
#include <sstream>
#include <iostream>
#include "Element.h"
// -----------------------------------------------------------------------------
namespace uniset
{
    // -------------------------------------------------------------------------
    using namespace std;
    // -------------------------------------------------------------------------
    const Element::ElementID Element::DefaultElementID = "?id?";
    // -------------------------------------------------------------------------

    void Element::addChildOut( std::shared_ptr<Element>& el, size_t num )
    {
        if( el.get() == this )
        {
            ostringstream msg;
            msg << "(" << myid << "): ПОПТКА СДЕЛАТь ССЫЛКУ НА САМОГО СЕБЯ!!!";
            throw LogicException(msg.str());
        }


        for( const auto& it : outs )
        {
            if( it.el.get() == el.get() )
            {
                ostringstream msg;
                msg << "(" << myid << "):" << el->getId() << " уже есть в списке дочерних(такое соединение уже есть)...";
                throw LogicException(msg.str());
            }
        }

        // проверка на циклическую зависимость
        // el не должен содержать в своих потомках myid
        if( el->find(myid) != NULL )
        {
            ostringstream msg;
            msg << "(" << myid << "):  ПОПЫТКА СОЗДАТЬ ЦИКЛИЧЕКУЮ ЗАВИСИМОСТЬ!!!\n";
            msg << " id" << el->getId() << " имеет в своих 'потомках' Element id=" << myid << endl;
            throw LogicException(msg.str());
        }

        outs.emplace_front(el, num);
    }
    // -------------------------------------------------------------------------
    void Element::delChildOut( std::shared_ptr<Element>& el )
    {
        for( auto it = outs.begin(); it != outs.end(); ++it )
        {
            if( it->el.get() == el.get() )
            {
                outs.erase(it);
                return;
            }
        }
    }
    // -------------------------------------------------------------------------
    size_t Element::outCount() const
    {
        return outs.size();
    }

    // -------------------------------------------------------------------------
    void Element::setChildOut()
    {
        long _myout = getOut();

        for( auto&& it : outs )
            it.el->setIn(it.num, _myout);
    }
    // -------------------------------------------------------------------------
    Element::ElementID Element::getId() const
    {
        return myid;
    }
    // -------------------------------------------------------------------------
    std::shared_ptr<Element> Element::find( const ElementID& id )
    {
        for( const auto& it : outs )
        {
            if( it.el->getId() == id )
                return it.el;

            return it.el->find(id);
        }

        return nullptr;
    }
    // -------------------------------------------------------------------------
    void Element::addInput(size_t num, long value )
    {
        for( auto&& it : ins )
        {
            if( it.num == num )
            {
                ostringstream msg;
                msg << "(" << myid << "): попытка второй раз добавить input N" << num;
                throw LogicException(msg.str());
            }
        }

        ins.emplace_front(num, value);
    }
    // -------------------------------------------------------------------------
    void Element::delInput( size_t num )
    {
        for( auto it = ins.begin(); it != ins.end(); ++it )
        {
            if( it->num == num )
            {
                ins.erase(it);
                return;
            }
        }
    }
    // -------------------------------------------------------------------------
    size_t Element::inCount() const
    {
        return ins.size();
    }
    // -------------------------------------------------------------------------
    ostream& operator<<( ostream& os, const Element& el )
    {
        return os << "[" << el.getType() << "]" << el.getId();
    }

    ostream& operator<<( ostream& os, const std::shared_ptr<Element>& el )
    {
        if( el )
            return os << (*(el.get()));

        return os;
    }
    // -------------------------------------------------------------------------
    long TOR::getOut() const
    {
        return (myout ? 1 : 0);
    }

    // -------------------------------------------------------------------------
} // end of namespace uniset
