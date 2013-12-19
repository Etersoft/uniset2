/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
// --------------------------------------------------------------------------
/*! \file
 * \brief Реализация IONotifyController_i
 * \author Pavel Vainerman
*/
// -------------------------------------------------------------------------- 
#ifndef IONotifyController_H_
#define IONotifyController_H_
//---------------------------------------------------------------------------
#include <map>
#include <list>
#include <string>

#include "UniSetTypes.h"
#include "IOController_i.hh"
#include "IOController.h"

//---------------------------------------------------------------------------
class NCRestorer;
//---------------------------------------------------------------------------
/*!
    \page page_IONotifyController Хранение информации о состоянии с уведомлениями об изменении (IONotifyController)
    
    Класс IONotifyController расширяет набор задач класса IOController.
    Для ознакомления с базовыми функциями см. \ref page_IOController

    Задачи решаемые IONotifyController-ом (\b IONC):
    - \ref sec_NC_AskSensors
    - \ref sec_NC_Consumers
    - \ref sec_NC_Thresholds
    - \ref sec_NC_Depends

    \section sec_NC_AskSensors Механизм заказа датчиков
    Главной задачей класса IONotifyController является уведомление
объектов (заказчиков) об изменении состояния датчика (входа или выхода).

Механизм функционирует по следующей логике: 
"заказчики" уведомляют \b IONC о том, об изменении какого именно датчика 
они хотят получать уведомление.
После чего, если данный датчик меняет своё состояние, заказчику посылается
сообщение UniSetTypes::SensorMessage содержащее информацию о текущем(новом) состоянии датчика,
времени изменения и т.п. В случае необходимости можно отказатся от уведомления.
Для заказа датчиков предусмотрен ряд функций. На данный момент рекомендуется
пользоватся функцией IONotifyController::askSensor. Функции askState и askValue считаются устаревшими
и оставлены для совместимости со старыми интерфейсами.
... продолжение следует...

    \section sec_NC_Consumers  Заказчики
В качестве "заказчиков" могут выступать любые UniSet-объекты (UniSetObject),
обладающие "обратным адресом" (идентификатором), по которому присылается
уведомление об изменении состояния. Свой обратный адрес, объекты указывают 
непосредственно при заказе (см. IONotifyController::askSensor).

Помимо "динамического" заказа во время работы процессов, существует возможность
задавать список заказчиков на этапе конфигурирования системы ("статический" способ). 
Для этого в конфигурационном файле, в секции \b <sensors> у каждого датчика предусмотрена
специальная секция \b <consumers>. 
\code
<sensors>
...
<item name="Sensors1" textname="sensor N1" iotype="AI" ...>
   <consumers>
       <consumer name="TestProc1" type="objects"/>
       <consumer name="TestProc2" type="managers" node="RemoteNode"/>
       ...
   </consumers>
</item>
...
</sensors>
\endcode
"Статический" способ заказа гарантирует, что при перезапуске
\b IONC список заказчиков будет восстановлен по конфигурационному файлу.
    
    \section sec_NC_Thresholds Пороговые датчики
    
    \section sec_NC_Depends Механизм зависимостей между датчиками
    Механизм зависимостей позволяет задать зависимость одного датчика, от другого.
Например пока "разрешающий" датчик не равен "1", у зависимого держится значение "0".
    Зависимость настраивается в конфигурационном файле, непосредственно у "зависимого датчика".
    <br>Доступные настройки:
  - \b depend            - задаёт имя датчика \b ОТ \b КОТОРОГО зависит данный
  - \b depend_value      - задаёт "разрешающее работу" значение (если поле не задано, считается depend_value="1").
  - \b depend_off_value  - задаёт значение которое будет выставлено для данного датчика, в случае его "блокировки". По умолчанию 0.

\code
    ...
    <sensors ..>
        ...
        <item id="11" iotype="AI" name="AI11_AS" textname="AI 11" depend="Input4_S" depend_off_value="-50"/>
        ...
    </sensors>
\endcode
    В данном примере можно увидеть, что датчик AI11_AS зависит от датчика Input4_S и пока Input4_S=0,
в AI11_AS будет записано значение -50. Как только Input4_S=1 в AI11_AS - появиться его истинное значение.

\note Следует иметь ввиду, что для \b ЗАВИСИМОГО датчика функция setValue(..) действует как обычно и
даже если он "заблокирован", значение в него можно сохранять. Оно "появиться" как только сниметься блокировка.
*/
//---------------------------------------------------------------------------
/*! \class IONotifyController
 * \todo Сделать логирование выходов 
 
 \section AskSensors Заказ датчиков
    
    ....
    ConsumerMaxAttempts - максимальное число неудачных 
попыток послать сообщение "заказчику". Настраивается в 
конфигурационном файле. По умолчанию = 5.
*/ 
class IONotifyController: 
    public IOController,
    public POA_IONotifyController_i
{
    public:
    
        IONotifyController(const std::string& name, const std::string& section, NCRestorer* dumper=0);
        IONotifyController(UniSetTypes::ObjectId id, NCRestorer* dumper=0);

        virtual ~IONotifyController();

        virtual UniSetTypes::ObjectType getType(){ return UniSetTypes::getObjectType("IONotifyController"); }

        virtual void askSensor(const IOController_i::SensorInfo& si, const UniSetTypes::ConsumerInfo& ci, UniversalIO::UIOCommand cmd);
        
        virtual void askThreshold(const IOController_i::SensorInfo& si, const UniSetTypes::ConsumerInfo& ci, 
                                    UniSetTypes::ThresholdId tid, 
                                    CORBA::Long lowLimit, CORBA::Long hiLimit, CORBA::Boolean invert, 
                                    UniversalIO::UIOCommand cmd );

        virtual IONotifyController_i::ThresholdInfo getThresholdInfo( const IOController_i::SensorInfo& si, UniSetTypes::ThresholdId tid );
        virtual IONotifyController_i::ThresholdList* getThresholds(const IOController_i::SensorInfo& si );
        virtual IONotifyController_i::ThresholdsListSeq* getThresholdsList();

        virtual UniSetTypes::IDSeq* askSensorsSeq(const UniSetTypes::IDSeq& lst, 
                                                    const UniSetTypes::ConsumerInfo& ci, UniversalIO::UIOCommand cmd);

        // --------------------------------------------

        // функция для работы напрямую черех iterator (оптимизация)
        virtual void localSetValue( IOController::IOStateList::iterator& it, 
                                    const IOController_i::SensorInfo& si,
                                    CORBA::Long value, UniSetTypes::ObjectId sup_id );

        // --------------------------------------------

        /*! Информация о заказчике */
        struct ConsumerInfoExt:
            public    UniSetTypes::ConsumerInfo
        {
            ConsumerInfoExt( const UniSetTypes::ConsumerInfo& ci,
                            UniSetObject_i_ptr ref=0, int maxAttemtps = 10 ):
                UniSetTypes::ConsumerInfo(ci),
                ref(ref),attempt(maxAttemtps){}

            UniSetObject_i_var ref;
            int attempt;
        };

        typedef std::list<ConsumerInfoExt> ConsumerList;

        /*! Информация о пороговом значении */
        struct ThresholdInfoExt:
            public IONotifyController_i::ThresholdInfo
        {
            ThresholdInfoExt( UniSetTypes::ThresholdId tid, CORBA::Long low, CORBA::Long hi, bool inv,
                                UniSetTypes::ObjectId _sid=UniSetTypes::DefaultObjectId ):
            sid(_sid),
            invert(inv)
            {
                id            = tid;
                hilimit        = hi;
                lowlimit    = low;
                state         = IONotifyController_i::NormalThreshold;
            }

            ConsumerList clst;

            /*! идентификатор дискретного датчика связанного с данным порогом */
            UniSetTypes::ObjectId sid;
            
            /*! итератор в списке датчиков (для оптимально-быстрого доступа) */
            IOController::IOStateList::iterator sit;
            
            /*! инверсная логика */
            bool invert; 
    
            inline bool operator== ( const ThresholdInfo& r ) const
            {
                return ((id == r.id) && 
                        (hilimit == r.hilimit) && 
                        (lowlimit == r.lowlimit) &&
                        (invert == r.invert) );
            }

            operator IONotifyController_i::ThresholdInfo()
            {
                IONotifyController_i::ThresholdInfo r;
                r.id = id;
                r.hilimit = hilimit;
                r.lowlimit = lowlimit;
                r.invert = invert;
                r.tv_sec = tv_sec;
                r.tv_usec = tv_usec;
                r.state = state;
                return r;
            }
        };
        
        typedef std::list<ThresholdInfoExt> ThresholdExtList;

        /*! массив пар датчик->список потребителей */
        typedef std::map<UniSetTypes::KeyType,ConsumerList> AskMap;
        
        struct ThresholdsListInfo
        {
            ThresholdsListInfo(){}
            ThresholdsListInfo(    IOController_i::SensorInfo& si, ThresholdExtList& list, 
                                UniversalIO::IOType t=UniversalIO::AI ):
                si(si),type(t),list(list){}
        
            IOController_i::SensorInfo si;
            IOStateList::iterator ait;
            UniversalIO::IOType type;
            ThresholdExtList list;
        };
        
        /*! массив пар [датчик,список порогов] */
        typedef std::map<UniSetTypes::KeyType,ThresholdsListInfo> AskThresholdMap;

    protected:
        IONotifyController();
        virtual bool activateObject();
        virtual void initItem( IOStateList::iterator& it, IOController* ic );

        // ФИЛЬТРЫ
        bool myIOFilter(const USensorInfo& ai, CORBA::Long newvalue, UniSetTypes::ObjectId sup_id);

        //! посылка информации об изменении состояния датчика
        virtual void send(ConsumerList& lst, UniSetTypes::SensorMessage& sm);

        //! проверка срабатывания пороговых датчиков
        virtual void checkThreshold( IOStateList::iterator& li, 
                                    const IOController_i::SensorInfo& si, bool send=true );

        //! поиск информации о пороговом датчике
        ThresholdExtList::iterator findThreshold( UniSetTypes::KeyType k, UniSetTypes::ThresholdId tid );
        
        //! сохранение информации об изменении состояния датчика в базу
        virtual void loggingInfo(UniSetTypes::SensorMessage& sm);

        /*! сохранение списка заказчиков 
            По умолчанию делает dump, если объявлен dumper.
        */
        virtual void dumpOrdersList(const IOController_i::SensorInfo& si, const IONotifyController::ConsumerList& lst);

        /*! сохранение списка заказчиков пороговых датчиков
            По умолчанию делает dump, если объявлен dumper.
        */
        virtual void dumpThresholdList(const IOController_i::SensorInfo& si, const IONotifyController::ThresholdExtList& lst);

        /*! чтение dump-файла */
        virtual void readDump();

        NCRestorer* restorer;

        void onChangeUndefinedState( IOStateList::iterator& it, IOController* ic );

//        UniSetTypes::uniset_rwmutex sig_mutex;
//        ChangeSignal changeSignal;

    private:
        friend class NCRestorer;

        //----------------------
        bool addConsumer(ConsumerList& lst, const UniSetTypes::ConsumerInfo& cons );     //!< добавить потребителя сообщения
        bool removeConsumer(ConsumerList& lst, const UniSetTypes::ConsumerInfo& cons );    //!< удалить потребителя сообщения
        
        //! обработка заказа 
        void ask(AskMap& askLst, const IOController_i::SensorInfo& si, 
                    const UniSetTypes::ConsumerInfo& ci, UniversalIO::UIOCommand cmd);        

         /*! добавить новый порог для датчика */
        bool addThreshold(ThresholdExtList& lst, ThresholdInfoExt& ti, const UniSetTypes::ConsumerInfo& cons);
        /*! удалить порог для датчика */
        bool removeThreshold(ThresholdExtList& lst, ThresholdInfoExt& ti, const UniSetTypes::ConsumerInfo& ci);


        AskMap askIOList; /*!< список потребителей по аналоговым датчикам */
        AskThresholdMap askTMap; /*!< список порогов по аналоговым датчикам */
        
        /*! замок для блокирования совместного доступа к cписку потребителей датчиков */
        UniSetTypes::uniset_rwmutex askIOMutex;
        /*! замок для блокирования совместного доступа к cписку потребителей пороговых датчиков */            
        UniSetTypes::uniset_rwmutex trshMutex;
        
        int maxAttemtps; /*! timeout for consumer */
};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------

