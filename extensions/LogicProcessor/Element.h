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
// --------------------------------------------------------------------------
#ifndef Element_H_
#define Element_H_
// --------------------------------------------------------------------------
#include <memory>
#include <string>
#include <list>
#include <ostream>
#include "Exceptions.h"
//--------------------------------------------------------------------------
namespace uniset
{
    // --------------------------------------------------------------------------

    class LogicException:
        public uniset::Exception
    {
        public:
            LogicException(): uniset::Exception("LogicException") {}
            explicit LogicException( const std::string& err): uniset::Exception(err) {}
    };


    class Element
    {
        public:

            typedef std::string ElementID;
            static const ElementID DefaultElementID;

            enum InputType
            {
                unknown,
                external,
                internal
            };

            explicit Element( const ElementID& id, bool _init = false ): myid(id),init_out(_init) {};
            virtual ~Element() {};


            /*! функция вызываемая мастером для элементов, которым требуется
                работа во времени.
                По умолчанию ничего не делает.
            */
            virtual void tick() {}

            virtual void setIn( size_t num, long value ) = 0;
            virtual long getOut() const = 0;

            ElementID getId() const;

            virtual std::string getType() const
            {
                return "?type?";
            }

            virtual std::shared_ptr<Element> find( const ElementID& id );

            virtual void addChildOut( std::shared_ptr<Element>& el, size_t in_num );
            virtual void delChildOut( std::shared_ptr<Element>& el );
            size_t outCount() const;

            virtual void addInput( size_t num, long value = 0 );
            virtual void delInput( size_t num );
            size_t inCount() const;

            friend std::ostream& operator<<(std::ostream& os, const Element& el );
            friend std::ostream& operator<<(std::ostream& os, const std::shared_ptr<Element>& el );

        protected:
            Element(): myid(DefaultElementID) {}; // нельзя создать элемент без id

            struct ChildInfo
            {
                ChildInfo(std::shared_ptr<Element> e, size_t n):
                    el(e), num(n) {}
                ChildInfo(): el(0), num(0) {}

                std::shared_ptr<Element> el;
                size_t num;
            };

            typedef std::list<ChildInfo> OutputList;
            OutputList outs;
            virtual void setChildOut();

            struct InputInfo
            {
                InputInfo(): num(0), value(0), type(unknown) {}
                InputInfo(size_t n, long v): num(n), value(v), type(unknown) {}
                size_t num;
                long value;
                InputType type;
            };

            typedef std::list<InputInfo> InputList;
            InputList ins;

            ElementID myid;
            bool init_out;

        private:
    };
    // ---------------------------------------------------------------------------
    class TOR:
        public Element
    {

        public:
            TOR( ElementID id, size_t numbers = 0, bool outstate = false );
            virtual ~TOR();

            virtual void setIn( size_t num, long value ) override;
            virtual long getOut() const override;

            virtual std::string getType() const override
            {
                return "OR";
            }

        protected:
            TOR(): myout(false) {}
            bool myout;


        private:
    };
    // ---------------------------------------------------------------------------
    class TAND:
        public TOR
    {

        public:
            TAND(ElementID id, size_t numbers = 0, bool st = false );
            virtual ~TAND();

            virtual void setIn( size_t num, long value ) override;
            virtual std::string getType() const override
            {
                return "AND";
            }

        protected:
            TAND() {}

        private:
    };

    // ---------------------------------------------------------------------------
    // элемент с одним входом и выходом
    class TNOT:
        public Element
    {

        public:
            TNOT( ElementID id, bool out_default );
            virtual ~TNOT();

            virtual long getOut() const override
            {
                return ( myout ? 1 : 0 );
            }

            /*! num игнорируется, т.к. элемент с одним входом */
            virtual void setIn( size_t num, long value ) override ;
            virtual std::string getType() const override
            {
                return "NOT";
            }
            virtual void addInput( size_t num, long value = 0 ) override {}
            virtual void delInput( size_t num ) override {}

        protected:
            TNOT(): myout(false) {}
            bool myout;

        private:
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// ---------------------------------------------------------------------------
#endif
