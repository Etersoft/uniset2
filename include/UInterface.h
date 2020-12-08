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
/*! \file
 * \brief Универсальный интерфейс для взаимодействия с объектами системы
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef UInterface_H_
#define UInterface_H_
// ---------------------------------------------------------------------------
#include <memory>
#include <string>
#include <atomic>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <omniORB4/CORBA.h>
#include "Exceptions.h"
#include "UniSetTypes.h"
#include "ObjectIndex.h"
#include "ObjectRepository.h"
#include "IOController_i.hh"
#include "MessageType.h"
#include "Configuration.h"
#ifndef DISABLE_REST_API
#include "UHttpClient.h"
#endif
// -----------------------------------------------------------------------------------------
namespace uniset
{
    /*!
     * \class UInterface
     * Универсальный интерфейс для взаимодействия между объектами (процессами).
     * По сути является "фасадом" к реализации механизма взаимодействия
     * в libuniset (основанном на CORBA) Хотя до конца скрыть CORBA-у пока не удалось.
     * Для увеличения производительности в функции встроен cache обращений...
     *
     * См. также \ref UniversalIOControllerPage
    */
    class UInterface
    {
        public:

            UInterface( const uniset::ObjectId backid, CORBA::ORB_var orb = NULL, const std::shared_ptr<uniset::ObjectIndex> oind = nullptr );
            UInterface( const std::shared_ptr<uniset::Configuration>& uconf = uniset::uniset_conf() );
            ~UInterface();

            // ---------------------------------------------------------------
            // Работа с датчиками

            //! Получение состояния датчика
            long getValue (const uniset::ObjectId id, const uniset::ObjectId node) const;
            long getValue ( const uniset::ObjectId id ) const;
            long getRawValue( const IOController_i::SensorInfo& si );

            //! Выставление состояния датчика
            void setValue ( const uniset::ObjectId id, long value, const uniset::ObjectId node, uniset::ObjectId sup_id = uniset::DefaultObjectId ) const;
            void setValue ( const uniset::ObjectId id, long value ) const;
            void setValue ( const IOController_i::SensorInfo& si, long value, const uniset::ObjectId supplier ) const;

            // fast - это удалённый вызов "без подтверждения", он быстрее, но менее надёжен
            // т.к. вызывающий никогда не узнает об ошибке, если она была (датчик такой не найден и т.п.)
            void fastSetValue( const IOController_i::SensorInfo& si, long value, uniset::ObjectId supplier ) const;

            //! Получение состояния для списка указанных датчиков
            IOController_i::SensorInfoSeq_var getSensorSeq( const uniset::IDList& lst );

            /*! Изменения состояния списка входов/выходов
                \return Возвращает список не найденных идентификаторов */
            uniset::IDSeq_var setOutputSeq( const IOController_i::OutSeq& lst, uniset::ObjectId sup_id );

            // ---------------------------------------------------------------
            // Заказ датчиков

            //! Универсальный заказ информации об изменении датчика
            void askSensor( const uniset::ObjectId id, UniversalIO::UIOCommand cmd,
                            uniset::ObjectId backid = uniset::DefaultObjectId ) const;

            void askRemoteSensor( const uniset::ObjectId id, UniversalIO::UIOCommand cmd, const uniset::ObjectId node,
                                  uniset::ObjectId backid = uniset::DefaultObjectId ) const;

            //! Заказ по списку
            uniset::IDSeq_var askSensorsSeq( const uniset::IDList& lst, UniversalIO::UIOCommand cmd,
                                             uniset::ObjectId backid = uniset::DefaultObjectId );
            // ------------------------------------------------------

            // установка неопределённого состояния
            void setUndefinedState( const IOController_i::SensorInfo& si, bool undefined, uniset::ObjectId supplier );

            // заморозка значения (выставить указанный value и не менять)
            void freezeValue( const IOController_i::SensorInfo& si, bool set, long value, uniset::ObjectId supplier = uniset::DefaultObjectId );
            // ---------------------------------------------------------------
            // Калибровка... пороги...

            //! калибровка
            void calibrate(const IOController_i::SensorInfo& si,
                           const IOController_i::CalibrateInfo& ci,
                           uniset::ObjectId adminId = uniset::DefaultObjectId );

            IOController_i::CalibrateInfo getCalibrateInfo( const IOController_i::SensorInfo& si );

            //! Заказ информации об изменении порога
            void askThreshold( const uniset::ObjectId sensorId, const uniset::ThresholdId tid,
                               UniversalIO::UIOCommand cmd,
                               long lowLimit, long hiLimit, bool invert = false,
                               uniset::ObjectId backid = uniset::DefaultObjectId ) const;

            void askRemoteThreshold( const uniset::ObjectId sensorId, const uniset::ObjectId node,
                                     const uniset::ThresholdId thresholdId, UniversalIO::UIOCommand cmd,
                                     long lowLimit, long hiLimit, bool invert = false,
                                     uniset::ObjectId backid = uniset::DefaultObjectId ) const;

            IONotifyController_i::ThresholdInfo getThresholdInfo( const IOController_i::SensorInfo& si, const uniset::ThresholdId tid ) const;
            IONotifyController_i::ThresholdInfo getThresholdInfo( const uniset::ObjectId sid, const uniset::ThresholdId tid ) const;

            // ---------------------------------------------------------------
            // Вспомогательные функции

            UniversalIO::IOType getIOType(const uniset::ObjectId id, uniset::ObjectId node) const;
            UniversalIO::IOType getIOType(const uniset::ObjectId id) const;

            // read from xml (only for xml!) т.е. без удалённого запроса
            UniversalIO::IOType getConfIOType( const uniset::ObjectId id ) const noexcept;

            // Получение типа объекта..
            uniset::ObjectType getType(const uniset::ObjectId id, const uniset::ObjectId node) const;
            uniset::ObjectType getType(const uniset::ObjectId id) const;

            //! Время последнего изменения датчика
            IOController_i::ShortIOInfo getTimeChange( const uniset::ObjectId id, const uniset::ObjectId node ) const;

            //! Информация об объекте
            std::string getObjectInfo( const uniset::ObjectId id, const std::string& params, const uniset::ObjectId node ) const;
            std::string apiRequest( const uniset::ObjectId id, const std::string& query, const uniset::ObjectId node ) const;

            //! Получить список датчиков
            IOController_i::ShortMapSeq* getSensors( const uniset::ObjectId id,
                    const uniset::ObjectId node = uniset::uniset_conf()->getLocalNode() );

            IOController_i::SensorInfoSeq* getSensorsMap( const uniset::ObjectId id,
                    const uniset::ObjectId node = uniset::uniset_conf()->getLocalNode() );

            IONotifyController_i::ThresholdsListSeq* getThresholdsList( const uniset::ObjectId id,
                    const uniset::ObjectId node = uniset::uniset_conf()->getLocalNode() );
            // ---------------------------------------------------------------
            // Работа с репозиторием

            /*! регистрация объекта в репозитории
             *  throw(uniset::ORepFailed)
             */
            void registered(const uniset::ObjectId id, const uniset::ObjectPtr oRef, bool force = false) const;

            // throw(uniset::ORepFailed)
            void unregister(const uniset::ObjectId id);

            /*! получение ссылки на объект */
            inline uniset::ObjectPtr resolve( const std::string& name ) const
            {
                return rep.resolve(name);
            }

            inline uniset::ObjectPtr resolve( const uniset::ObjectId id ) const
            {
                return rep.resolve( oind->getNameById(id) );
            }

            // throw(uniset::ResolveNameError, uniset::TimeOut);
            uniset::ObjectPtr resolve(const uniset::ObjectId id, const uniset::ObjectId nodeName) const;

            // Проверка доступности объекта или датчика
            bool isExist( const uniset::ObjectId id ) const noexcept;
            bool isExist( const uniset::ObjectId id, const uniset::ObjectId node ) const noexcept;

            //! used for check 'isExist' \deprecated! Use waitReadyWithCancellation(..)
            bool waitReady( const uniset::ObjectId id, int msec, int pause = 5000,
                            const uniset::ObjectId node = uniset::uniset_conf()->getLocalNode() ) noexcept;

            //! used for check 'getValue'
            bool waitWorking( const uniset::ObjectId id, int msec, int pause = 3000,
                              const uniset::ObjectId node = uniset::uniset_conf()->getLocalNode() ) noexcept;

            bool waitReadyWithCancellation( const uniset::ObjectId id, int msec, std::atomic_bool& cancelFlag, int pause = 5000,
                                            const uniset::ObjectId node = uniset::uniset_conf()->getLocalNode() ) noexcept;

            // ---------------------------------------------------------------
            // Работа с ID, Name

            /*! получение идентификатора объекта по имени */
            inline uniset::ObjectId getIdByName( const std::string& name ) const noexcept
            {
                return oind->getIdByName(name);
            }

            /*! получение имени по идентификатору объекта */
            inline std::string getNameById( const uniset::ObjectId id ) const noexcept
            {
                return oind->getNameById(id);
            }

            inline uniset::ObjectId getNodeId( const std::string& fullname ) const noexcept
            {
                return oind->getNodeId(fullname);
            }

            inline std::string getTextName( const uniset::ObjectId id ) const noexcept
            {
                return oind->getTextName(id);
            }

            // ---------------------------------------------------------------
            // Получение указателей на вспомогательные классы.
            inline const std::shared_ptr<uniset::ObjectIndex> getObjectIndex() noexcept
            {
                return oind;
            }
            inline const std::shared_ptr<uniset::Configuration> getConf() noexcept
            {
                return uconf;
            }
            // ---------------------------------------------------------------
            // Посылка сообщений

            /*! посылка сообщения msg объекту name на узел node */
            void send( const uniset::ObjectId name, const uniset::TransportMessage& msg, uniset::ObjectId node );
            void send( const uniset::ObjectId name, const uniset::TransportMessage& msg);
            void sendText(const uniset::ObjectId name, const std::string& text, int mtype, const uniset::ObjectId node = uniset::DefaultObjectId );
            void sendText(const uniset::ObjectId name, const uniset::TextMessage& msg, const uniset::ObjectId node = uniset::DefaultObjectId );

            // ---------------------------------------------------------------
            // Вспомогательный класс для кэширования ссылок на удалённые объекты

            inline void setCacheMaxSize( size_t newsize ) noexcept
            {
                rcache.setMaxSize(newsize);
            }

            /*! Кэш ссылок на объекты */
            class CacheOfResolve
            {
                public:
                    CacheOfResolve( size_t maxsize, size_t cleancount = 20 ):
                        MaxSize(maxsize), minCallCount(cleancount) {};
                    ~CacheOfResolve() {};

                    //  throw(uniset::NameNotFound, uniset::SystemError)
                    uniset::ObjectPtr resolve( const uniset::ObjectId id, const uniset::ObjectId node ) const;

                    void cache(const uniset::ObjectId id, const uniset::ObjectId node, uniset::ObjectVar& ptr ) const;
                    void erase( const uniset::ObjectId id, const uniset::ObjectId node ) const noexcept;

                    inline void setMaxSize( size_t ms ) noexcept
                    {
                        MaxSize = ms;
                    };

                protected:
                    CacheOfResolve() {};

                private:

                    bool clean() noexcept;       /*!< функция очистки кэш-а от старых ссылок */
                    inline void clear() noexcept /*!< удаление всей информации */
                    {
                        uniset::uniset_rwmutex_wrlock l(cmutex);
                        mcache.clear();
                    };

                    struct Item
                    {
                        Item( const uniset::ObjectVar& ptr ): ptr(ptr), ncall(0) {}
                        Item(): ptr(NULL), ncall(0) {}

                        uniset::ObjectVar ptr;
                        size_t ncall; // счётчик обращений

                        bool operator<( const CacheOfResolve::Item& rhs ) const
                        {
                            return this->ncall > rhs.ncall;
                        }
                    };

                    typedef std::unordered_map<uniset::KeyType, Item> CacheMap;
                    mutable CacheMap mcache;
                    mutable uniset::uniset_rwmutex cmutex;
                    size_t MaxSize = { 20 };      /*!< максимальный размер кэша */
                    size_t minCallCount = { 20 }; /*!< минимальное количество вызовов, меньше которого ссылка считается устаревшей */
            };

            void initBackId( uniset::ObjectId backid );

        protected:
            std::string set_err(const std::string& pre, const uniset::ObjectId id, const uniset::ObjectId node) const;
            std::string httpResolve( const uniset::ObjectId id, const uniset::ObjectId node ) const;

        private:
            void init();

            ObjectRepository rep;
            uniset::ObjectId myid;
            mutable CosNaming::NamingContext_var localctx;
            mutable CORBA::ORB_var orb;
            CacheOfResolve rcache;
            std::shared_ptr<uniset::ObjectIndex> oind;
            std::shared_ptr<uniset::Configuration> uconf;
    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
