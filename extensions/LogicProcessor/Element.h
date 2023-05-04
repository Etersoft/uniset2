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
            TOR( ElementID id, size_t numbers = 0, bool st = false );
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
            TNOT( ElementID id, bool st = false );
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
    /* элемент с одним управляющим входом, двумя входными параметрами и одним выходом.
    * выход выбирается между двумя входными параметрами по состоянию управляющего входа:
    * Входные параметры(ВП 1 и 2) - целочисленные переменные, управляющий вход(УВ) - булева переменная,
    * выход(ВЫХ) - целочисленная переменная.
    * Значения входные параметров задаются статически при создании элемента через
    * параметры sel_true и sel_false, а также можно привязать внешние входы.
    * 
    * Пример таблицы:
    *  
    *  ВП1(true_inp) | ВП2(false_inp) | УВ | ВЫХ
    *       50       |      100       | 0  | 100
    *       50       |      100       | 1  | 50

    *
    */
    class TSEL_R:
        public Element
    {

        public:
            TSEL_R( ElementID id, bool st = false, long _sel_false = 0, long _sel_true = 1 );
            virtual ~TSEL_R();

            virtual long getOut() const override
            {
                return myout;
            }

            /*! num игнорируется, т.к. элемент с одним входом */
            virtual void setIn( size_t num, long value ) override ;
            virtual std::string getType() const override
            {
                return "SEL_R";
            }
            virtual void addInput( size_t num, long value = 0 ) override {}
            virtual void delInput( size_t num ) override {}

        protected:
            TSEL_R(): myout(0), control_inp(false), false_inp(0), true_inp(1) {}
            long myout;       /*<! Selected value */
            bool control_inp; /*<! Input selection */
            long true_inp;    /*<! Input  value 1 when control_inp=true */
            long false_inp;   /*<! Input  value 2 when control_inp=false */

        private:
    };
    // --------------------------------------------------------------------------
    /* Элемент с двумя входами и одним выходом.
    * первый вход для выставления выхода в true.
    * второй вход для сброса выхода в false.
    * На вход подается только логическая единица
    * ,а ноль игнорируется.
    * Таблица истинности:
    *  
    *  вход 1(set) | вход 2(reset) | выход(out)
    *    0         |    0          |   значение по-умолчанию или предыдущее значение выхода
    *    0         |    1          |   0
    *    1         |    0          |   1
    *    1         |    1          |   доминантный вход
    *
    */
    class TRS:
        public Element
    {

        public:
            TRS( ElementID id, bool st = false, bool _dominantReset = false );
            virtual ~TRS();

            virtual long getOut() const override
            {
                return ( myout ? 1 : 0 );
            }

            virtual void setIn( size_t num, long value ) override ;
            virtual std::string getType() const override
            {
                return "RS";
            }
            virtual void addInput( size_t num, long value = 0 ) override {}
            virtual void delInput( size_t num ) override {}

        protected:
            TRS(): myout(false), dominantReset(false), set_inp(false), reset_inp(false) {}
            bool myout;         /*<! Output */
            bool dominantReset; /*<! Dominant reset input (by default: set input is dominant) */
            bool set_inp;       /*<! Set input */
            bool reset_inp;     /*<! Reset input */

        private:
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// ---------------------------------------------------------------------------
#endif
