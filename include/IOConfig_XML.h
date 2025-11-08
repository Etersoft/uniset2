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
 * \brief Интерфейс к объекту сохраняющему список заказчиков для NotifyController-ов
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef IOConfig_XML_H_
#define IOConfig_XML_H_
// ------------------------------------------------------------------------------------------
#include <memory>
#include <sigc++/sigc++.h>
#include <string>
#include "UniXML.h"
#include "UniSetTypes.h"
#include "IOConfig.h"
#include "AccessConfig.h"
// ------------------------------------------------------------------------------------------
namespace uniset
{
    // ------------------------------------------------------------------------------------------
    /*! Реализация интерфейса конфигурирования на основе XML */
    class IOConfig_XML final:
        public IOConfig
    {
        public:
            virtual ~IOConfig_XML();

            // реализация интерфейса IOConfig
            virtual IOController::IOStateList read() override;

            // читать список датчиков
            static ACLInfoMap readACLInfo( const std::shared_ptr<Configuration>& conf, const std::shared_ptr<UniXML>& _xml );

            /*!
             * \param fname - файл формата uniset-project
             * \param conf  - конфигурация
             */
            IOConfig_XML( const std::string& fname, const std::shared_ptr<Configuration>& conf );

            /*!
             * \param xml  - xml формата uniset-project
             * \param conf - конфигурация
             * \param root - узел со списком датчиков (если не задан, ищется "sensors")
             */
            IOConfig_XML( const std::shared_ptr<UniXML>& _xml, const std::shared_ptr<Configuration>& conf, xmlNode* root = nullptr );

            void setAclConfig( const std::string& name, const std::string& section="ACLConfig");
            void setAclIgnoreError( bool st );

            /*! слот для подключения функции чтения датчика из xml-файла.
                \param uxml  - интерфейс для работы с xml-файлом
                \param it    - итератор (указатель) на текущий считываемый xml-узел (item)
                \param sec   - итератор (указатель) на корневой узел секции (SubscriberList)
                \return TRUE - если чтение параметров прошло успешно, FALSE - если нет
            */
            typedef sigc::slot<bool, const std::shared_ptr<UniXML>&, UniXML::iterator&, xmlNode*> ReaderSlot;

            /*! установить функцию для callback-вызова при чтении списка сообщений
                For example:
                    setReadItem( sigc::mem_fun(this,&MyClass::myReadItem) );

                bool myReadItem::myfunc(UniXML& xml,
                                            UniXML::iterator& it, xmlNode* sec)

                uxml  - интерфейс для работы с xml-файлом
                it    - итератор(указатель) на текущий считываемый xml-узел (item)
                sec   - указатель на корневой узел секции (SubscriberList)
            */
            void setReadItem( ReaderSlot sl );


            /*! установить функцию для callback-вызова при чтении списка заказчиков
                For example:
                    setReadItem( sigc::mem_fun(this,&MyClass::myReadItem) );

                bool myReadItem::myfunc(UniXML& xml,
                                            UniXML::iterator& it, xmlNode* sec)

                uxml  - интерфейс для работы с xml-файлом
                it    - итератор(указатель) на текущий считываемый xml-узел (<consumer>)
                sec   - указатель на текущий узел сообщения (<item>)
            */
            void setReadConsumerItem( ReaderSlot sl );

            /*! Установить фильтр на чтение списка датчиков
                \note Функцию необходимо вызывать до вызова read(...)
             */
            void setItemFilter( const std::string& filterField, const std::string& filterValue = "" );

            /*! Установить фильтр на чтение списка заказчиков (по каждому датчику)
                \note Функцию необходимо вызывать до вызова read(...)
             */
            void setConsumerFilter( const std::string& filterField, const std::string& filterValue = "" );


            /*! универсальная функция получения информации о заказчике (id и node)
                по новому формату файла (<consumer name="xxxx" type="objects" />)
                \return true - если идентификаторы определены
            */
            bool getConsumerInfo( UniXML::iterator& it,
                                  uniset::ObjectId& cid, uniset::ObjectId& cnode ) const;

            /*! Установить фильтр на чтение списка 'порогов' */
            void setThresholdsFilter( const std::string& filterField, const std::string& filterValue = "" );

            /*! установить функцию для callback-вызова
                при чтении списка пороговых датчиков.

                bool xxxMyClass::myfunc(UniXML& xml,
                                        UniXML::iterator& it, xmlNode* sec)
                uxml   - интерфейс для работы с xml-файлом
                it     - итератор(указатель) на текущий считываемый xml-узел (<sensor>)
                sec    - указатель на корневой узел секции (<threshold>)
            */
            void setReadThresholdItem( ReaderSlot sl );


            typedef sigc::slot<bool, const std::shared_ptr<UniXML>&, UniXML::iterator&, xmlNode*, std::shared_ptr<IOController::USensorInfo>&> NCReaderSlot;

            /*! установить callback на событие формирования информации в формате IOController (USenorInfo) */
            void setNCReadItem( NCReaderSlot sl );

        protected:
            IOConfig_XML();

            bool check_list_item( UniXML::iterator& it ) const;
            bool check_consumer_item( UniXML::iterator& it ) const;
            bool check_thresholds_item( UniXML::iterator& it ) const;
            void read_consumers( const std::shared_ptr<UniXML>& xml, xmlNode* node, std::shared_ptr<IOController::USensorInfo>& inf );
            IOController::IOStateList read_list( xmlNode* node );
            void init_thresholds( xmlNode* node, IOController::IOStateList& iolist );
            void init_depends_signals( IOController::IOStateList& lst );

            bool getBaseInfo( xmlNode* it, IOController_i::SensorInfo& si ) const;
            bool getSensorInfo( xmlNode* snode, std::shared_ptr<IOController::USensorInfo>& si ) const;
            bool getThresholdInfo(xmlNode* tnode, std::shared_ptr<IOController::UThresholdInfo>& ti) const;
            //          bool getConsumerList( const std::shared_ptr<UniXML>& xml, xmlNode* node, IONotifyController::ConsumerListInfo& lst) const;

            static void set_dumptime( const std::shared_ptr<UniXML>& xml, xmlNode* node );
            static xmlNode* bind_node( const std::shared_ptr<UniXML>& xml, xmlNode* root, const std::string& nodename, const std::string& nm = "");
            static xmlNode* rebind_node( const std::shared_ptr<UniXML>& xml, xmlNode* root, const std::string& nodename, const std::string& nm = "");

            std::string s_filterField = { "" };
            std::string s_filterValue = { "" };
            std::string t_filterField = { "" };
            std::string t_filterValue = { "" };

            std::string i_filterField = { "" };
            std::string i_filterValue = { "" };
            std::string c_filterField = { "" };
            std::string c_filterValue = { "" };

            std::string fname = { "" };
            std::shared_ptr<Configuration> conf;
            std::shared_ptr<UniXML> uxml;
            uniset::ACLMap acls;
            std::string aclConfigName;
            std::string aclConfigSectionName;
            bool aclIgnoreErrors = { false };
            xmlNode* root = { nullptr };
            ReaderSlot rtslot;
            ReaderSlot rslot;
            ReaderSlot cslot;
            NCReaderSlot ncrslot;

        private:
    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
// -----------------------------------------------------------------------------
#endif // IOConfig_XML_H_
