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
 * \brief Интерфейс к объекту сохраняющему список заказчиков для NotifyController-ов
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef NCRestorer_H_
#define NCRestorer_H_
// ------------------------------------------------------------------------------------------
#include <memory>
#include <sigc++/sigc++.h>
#include <string>
#include "UniXML.h"
#include "Restorer.h"
#include "IOController.h"
#include "IONotifyController.h"
// ------------------------------------------------------------------------------------------
/*!
    Интерфейс для записи в файл и восстановления из файла списка заказчиков по датчикам для
    IONotifyController-а (NC).

    \note Это абстрактный интерфейс. В чистом виде не используется.
*/ 
class NCRestorer
{
    public:

        NCRestorer();
        virtual ~NCRestorer();

        struct SInfo:
            public IOController::USensorInfo
        {
            SInfo( const SInfo& ) = delete;
            const SInfo& operator=(const SInfo& ) = delete;
            SInfo( SInfo&& ) = default;
            SInfo& operator=(SInfo&& ) = default;

            SInfo( IOController_i::SensorInfo& si, UniversalIO::IOType& t,
                    UniSetTypes::Message::Message::Priority& p, long& def )
            {
                this->si = si;
                this->type = t;
                this->priority = p;
                this->default_val = def;
            }

            SInfo()
            {
                this->type = UniversalIO::DI;
                this->priority = UniSetTypes::Message::Medium;
                this->default_val = 0;
            }

            SInfo &operator=(const IOController_i::SensorIOInfo& inf);
            SInfo( const IOController_i::SensorIOInfo& inf );
        };

        virtual void read( IONotifyController* ic, const std::string& fn="" )=0;
        virtual void dump(const IONotifyController* ic, std::shared_ptr<SInfo>& inf, const IONotifyController::ConsumerListInfo& lst)=0;
        virtual void dumpThreshold(const IONotifyController* ic, std::shared_ptr<SInfo>& inf, const IONotifyController::ThresholdExtList& lst)=0;

    protected:

        // добавление списка заказчиков
        static void addlist( IONotifyController* ic, std::shared_ptr<IOController::USensorInfo>& inf, IONotifyController::ConsumerListInfo&& lst, bool force=false );

        // добавление списка порогов и заказчиков
        static void addthresholdlist( IONotifyController* ic, std::shared_ptr<IOController::USensorInfo>& inf, IONotifyController::ThresholdExtList&& lst, bool force=false );

        static inline void ioRegistration( IONotifyController* ic, std::shared_ptr<IOController::USensorInfo>& inf, bool force=false )
        {
            ic->ioRegistration(inf,force);
        }

        static inline IOController::IOStateList::iterator ioFind( IONotifyController* ic, UniSetTypes::KeyType k )
        {
            return ic->myiofind(k);
        }

        static inline IOController::IOStateList::iterator ioEnd( IONotifyController* ic )
        {
            return ic->myioEnd();
        }
        static inline IOController::IOStateList::iterator ioBegin( IONotifyController* ic )
        {
            return ic->myioBegin();
        }

        static void init_depends_signals( IONotifyController* ic );
};
// ------------------------------------------------------------------------------------------
/*!
 * \brief Реализация сохранения списка заказчиков в xml.
    Данный класс работает с глобальным xml-файлом проекта (обычно configure.xml),
    поэтому НЕ реализаует функции записи (dump)-а.
*/ 
class NCRestorer_XML:
    public Restorer_XML,
    public NCRestorer
{
    public:

        /*!
            \param fname - файл. (формата uniset-project)
        */
        NCRestorer_XML( const std::string& fname );

        /*!
            \param fname - файл. (формата uniset-project)
            \param sensor_filterField - читать из списка только те узлы, у которых filterField="filterValue"
            \param sensor_filterValue - значение для фильтрования списка
        */
        NCRestorer_XML( const std::string& fname, const std::string& sensor_filterField, const std::string& sensor_filterValue="" );

        virtual ~NCRestorer_XML();
        NCRestorer_XML();

        /*! Установить фильтр на чтение списка 'порогов' */
        void setThresholdsFilter( const std::string& filterField, const std::string& filterValue="" );

        bool setFileName( const std::string& file, bool create );
        inline std::string getFileName(){ return fname; }

        /*! установить функцию для callback-вызова
            при чтении списка пороговых датчиков.

            bool xxxMyClass::myfunc(UniXML& xml,
                                    UniXML::iterator& it, xmlNode* sec)
            uxml    - интерфейс для работы с xml-файлом
            it     - интератор(указатель) на текущий считываемый xml-узел (<sensor>)
            sec    - указатель на корневой узел секции (<threshold>)
        */
        void setReadThresholdItem( ReaderSlot sl );

        typedef sigc::slot<bool,const std::shared_ptr<UniXML>&,UniXML::iterator&,xmlNode*,std::shared_ptr<IOController::USensorInfo>&> NCReaderSlot;

        void setNCReadItem( NCReaderSlot sl );

        virtual void read( IONotifyController* ic, const std::string& filename="" );
        virtual void read( IONotifyController* ic, const std::shared_ptr<UniXML>& xml );

        virtual void dump(const IONotifyController* ic, std::shared_ptr<NCRestorer::SInfo>& inf, const IONotifyController::ConsumerListInfo& lst) override;
        virtual void dumpThreshold(const IONotifyController* ic, std::shared_ptr<NCRestorer::SInfo>& inf, const IONotifyController::ThresholdExtList& lst) override;

    protected:

        bool check_thresholds_item( UniXML::iterator& it );
        void read_consumers( const std::shared_ptr<UniXML>& xml, xmlNode* node, std::shared_ptr<NCRestorer_XML::SInfo>& inf, IONotifyController* ic );
        void read_list( const std::shared_ptr<UniXML>& xml, xmlNode* node, IONotifyController* ic);
        void read_thresholds( const std::shared_ptr<UniXML>& xml, xmlNode* node, IONotifyController* ic);
        void init( const std::string& fname );

        bool getBaseInfo( const std::shared_ptr<UniXML>& xml, xmlNode* it, IOController_i::SensorInfo& si );
        bool getSensorInfo( const std::shared_ptr<UniXML>& xml, xmlNode* snode, std::shared_ptr<NCRestorer_XML::SInfo>& si );
        bool getConsumerList( const std::shared_ptr<UniXML>& xml,xmlNode* node, IONotifyController::ConsumerListInfo& lst);
        bool getThresholdInfo(const std::shared_ptr<UniXML>& xml,xmlNode* tnode, IONotifyController::ThresholdInfoExt& ti);

        static void set_dumptime( const std::shared_ptr<UniXML>& xml, xmlNode* node );
        static xmlNode* bind_node( const std::shared_ptr<UniXML>& xml, xmlNode* root, const std::string& nodename, const std::string& nm="");
        static xmlNode* rebind_node( const std::shared_ptr<UniXML>& xml, xmlNode* root, const std::string& nodename, const std::string& nm="");

        std::string s_filterField;
        std::string s_filterValue;
        std::string c_filterField;
        std::string c_filterValue;
        std::string t_filterField;
        std::string t_filterValue;

        std::string fname;
        std::shared_ptr<UniXML> uxml;
        ReaderSlot rtslot;
        NCReaderSlot ncrslot;

    private:
};
// ------------------------------------------------------------------------------------------
#endif
