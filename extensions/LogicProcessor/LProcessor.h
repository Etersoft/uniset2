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
#ifndef LProcessor_H_
#define LProcessor_H_
// --------------------------------------------------------------------------
/*! \page pageLogicProcessor Логический процессор

    \section sec_lpCommon Общее описание
        Логический процессор представляет из себя процесс, который работает по принципу
    PLC-контроллеров, выполняя бесконечный цикл:
    \code
        1. опрос входов
        2. шаг алгоритма
        3. выставление выходов
    \endcode

        При этом "логика" формируется из простых логических элементов:

    -# \b "И" \b (AND)
    -# \b "ИЛИ" \b (OR)
    -# \b "Задержка" \b (Delay)
    -# \b "Отрицание" \b (NOT)
    -# \b "Преобразование аналогового значения в логическое" \b (A2D)

    Стоит отметить, что по мере развития, процесс стал поддерживать не только логические операции,
    а работу с числовыми(аналоговыми) величинами. Например элемент "TA2D",
    но в названии оставлено "Logic".

    \section sec_lpShema Конфигурирование
        Конфигурирование процесса осуществляется при помощи xml-файла задающего
    "схему соединения" элементов. Например
    \code
<Schema>
<text-view>
       ----
  1 --|    |
  2 --|TOR1|---|  1  -----
      |    |   |----|     |
       ----       2 |     |--|
               |----|TAND3|  |
       ----    |    |     |  |
      |    |   |     -----   |
  1 --|TOR2|   |             |            1  ----       -------
  2 --|    |---              |    ----   ---|    |     |       |  out
      |    |                 | 1 |    |   2 |TOR5|-----| Delay |----
       ----                  |---|TOR4|-----|    |     |       |
                           2 ----|    |     |    |     |       |
                                  ----       ----       -------
</text-view>
<elements>
    <item id="1" type="OR" inCount="2"/>
    <item id="2" type="OR" inCount="2"/>
    <item id="3" type="AND" inCount="2"/>
    <item id="4" type="OR" inCount="2"/>
    <item id="5" type="OR" inCount="2"/>
    <item id="6" type="Delay" inCount="1" delayMS="3000"/>
</elements>

<connections>
    <item type="ext" from="Input1_S" to="1" toInput="1" />
    <item type="ext" from="Input2_S" to="1" toInput="2" />
    <item type="ext" from="Input3_S" to="2" toInput="1" />
    <item type="ext" from="Input4_S" to="2" toInput="2" />
    <item type="ext" from="Input5_S" to="4" toInput="2" />
    <item type="ext" from="Input6_S" to="5" toInput="1" />
    <item type="int" from="1" to="3" toInput="1" />
    <item type="int" from="2" to="3" toInput="2" />
    <item type="int" from="3" to="4" toInput="1" />
    <item type="int" from="4" to="5" toInput="2" />
    <item type="int" from="5" to="6" toInput="1" />
    <item type="out" from="6" to="Output1_C"/>
</connections>
</Schema>
    \endcode

        Блок \b <elements> содержит список элементов участвующих в "логике", каждому из
    которых присвоен уникальный id, а также характеристики каждого элемента.
    В секции \b <connections> задаютcя собственно соединения.

    \par  Тэги:
    - \b type="ext" - Соединение связанное с внешним датчиком, задаваемым по имени.
    - \b type="int" - Внутреннее соединение элементов между собой.
    - \b type="out" - Замыкание на "внешний" датчик.
    - \b from=".." - задаёт идентификатор элемента ("откуда"). Для type="ext", сюда пишется ID датчика.
    - \b to=".." - задаёт идентификатор элемента("куда"), к которому происходит подключение.
    - \b toInput=".." - В качестве значения указывается номер "входа" элемента на который подаётся "сигнал".

    В текущей реализации в качестве датчиков разрешено использовать только типы DO или DI.

    \note Следует иметь ввиду, что схема \b не \b обязательно должна быть \b "СВЯЗАННОЙ"
    (все элементы связанны между собой). В файле может содержаться несколько схем внутри тэга \b <Schema>.
    Логика исполняется в порядке следования в файле, сверху вниз (в порядке считывания из файла).
*/
// --------------------------------------------------------------------------
#include <list>
#include <atomic>
#include "UniSetTypes.h"
#include "UInterface.h"
#include "Element.h"
#include "Schema.h"
// --------------------------------------------------------------------------
namespace uniset
{
    // --------------------------------------------------------------------------
    class LProcessor
    {
        public:
            explicit LProcessor( const std::string& name = "" );
            virtual ~LProcessor();

            void open( const std::string& lfile );

            bool isOpen() const;

            timeout_t getSleepTime() const noexcept;

            std::shared_ptr<SchemaXML> getSchema();

            virtual void execute( const std::string& lfile = "" );

            virtual void terminate();

        protected:

            virtual void build( const std::string& lfile );

            virtual void step();

            virtual void getInputs();
            virtual void processing();
            virtual void setOuts();

            struct EXTInfo
            {
                uniset::ObjectId sid = { uniset::DefaultObjectId };
                long value = { 0 };
                std::shared_ptr<Element> el = { nullptr };
                int numInput = { -1 };
            };

            struct EXTOutInfo
            {
                uniset::ObjectId sid = { uniset::DefaultObjectId };
                std::shared_ptr<Element> el = { nullptr };
            };

            typedef std::list<EXTInfo> EXTList;
            typedef std::list<EXTOutInfo> OUTList;

            EXTList extInputs;
            OUTList extOuts;

            std::shared_ptr<SchemaXML> sch;

            UInterface ui;
            timeout_t sleepTime = { 200 };
            timeout_t smReadyTimeout = { 120000 } ;     /*!< время ожидания готовности SM, мсек */

            std::string logname = { "" };
            std::atomic_bool canceled = {false};
            std::string fSchema = {""};

        private:
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// ---------------------------------------------------------------------------
#endif
