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
#ifndef UProxyObject_H_
#define UProxyObject_H_
// --------------------------------------------------------------------------
#include <memory>
#include "Configuration.h"
#include "UExceptions.h"
#include "UTypes.h"
// --------------------------------------------------------------------------
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wdeprecated"
// --------------------------------------------------------------------------
class UProxyObject_impl; // PIMPL
// --------------------------------------------------------------------------
/*! Интерфейс для взаимодействия с SM (с заказом датчиков).
 * Текущая версия рассчитана на наличие локальной SM, т.е. в функциях нет аргумента node.
 * Соответственно обращение к датчику на другом узле НЕВОЗМОЖНО.
 *
 * Если всё-таки понадобится - доработаем.
 *
 * Общее описание:
 * Перед активацией объекта необходимо при помощи addToAsk() добавить список датчиков,
 * за которыми требуется "следить". После активации ( см. PyUInterface uniset_activate_objects() )
 * (в асинхронном режиме!) объект заказывает датчики и сохраняет у себя их состояние.
 * При этом "снаружи" можно запросить значение ранее добавленного датчика при помощи функции getValue().
 * Помимо этого можно изменять состояние датчиков (в SM!) при помощи setValue().
 * По сути setValue() просто дублирует функциональность PyUInterface::setValue()
 */
class UProxyObject
{
    public:
        UProxyObject( const std::string& name ); // throw(UException);
        UProxyObject( long id ); // throw(UException);
        ~UProxyObject();

        //! \note Вызывать надо до активации объекта
        void addToAsk( long id ); // throw(UException);

        //! можно вызывать в процессе работы
        void askSensor( long id ); // throw(UException);

        long getValue( long id ); // throw(UException);
        float getFloatValue( long id ); // throw(UException);

        /*! Сохраняемые датчики не требуют добавления при помощи addToAsk ! */
        void setValue( long id, long val ); // throw(UException);

        /*! \return true если заказ датчиков прошёл успешно */
        bool askIsOK();

        /*! перезаказ датчиков */
        bool reaskSensors();

        /*! принудительное обновление значений.
         * В случае если не используется заказ датчиков
         */
        bool updateValues();

        /*! Проверка работы SM */
        bool smIsOK();

    protected:
        void init( long id ); // throw( UException );

    private:
        UProxyObject(); // throw(UException);

        std::shared_ptr<UProxyObject_impl> uobj;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
