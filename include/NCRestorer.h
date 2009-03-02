/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman <pv>
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
 * \author Pavel Vainerman <pv>
 * \version $Id: NCRestorer.h,v 1.17 2008/12/14 21:57:51 vpashka Exp $
 * \date $Date: 2008/12/14 21:57:51 $
 */
// --------------------------------------------------------------------------
#ifndef NCRestorer_H_
#define NCRestorer_H_
// ------------------------------------------------------------------------------------------
#include <sigc++/sigc++.h>
#include <string>
#include "UniXML.h"
#include "Restorer.h"
#include "IOController.h"
#include "IONotifyController.h"
// ------------------------------------------------------------------------------------------
/*!
	Это абстрактный интерфейс. В чистом виде не используется.
*/ 
class NCRestorer
{
	public:

		NCRestorer();
	    virtual ~NCRestorer();

		struct SInfo:
			public IOController::UniAnalogIOInfo
		{
			SInfo( IOController_i::SensorInfo& si, UniversalIO::IOTypes& t,
					UniSetTypes::Message::Message::Priority& p, long& def )
			{
				this->si = si;
				this->type = t;
				this->priority = p;
				this->default_val = def;
			}

			SInfo()
			{
				this->type = UniversalIO::DigitalInput;
				this->priority = UniSetTypes::Message::Medium;
				this->default_val = 0;
			}

			SInfo &operator=(IOController_i::DigitalIOInfo& inf);
			SInfo &operator=(IOController_i::AnalogIOInfo& inf);
			
			operator IOController::UniDigitalIOInfo();
		};

		virtual void read(IONotifyController* ic, const std::string fn="" )=0;
		virtual void buildDependsList( IONotifyController* ic, const std::string fn="" )=0;
		virtual void dump(IONotifyController* ic, SInfo& inf, const IONotifyController::ConsumerList& lst)=0;
		virtual void dumpThreshold(IONotifyController* ic, SInfo& inf, const IONotifyController::ThresholdExtList& lst)=0;

	protected:

		// добавление списка заказчиков
		static void addlist( IONotifyController* ic, SInfo& inf, IONotifyController::ConsumerList& lst, bool force=false );

		// добавление списка порогов и заказчиков
		static void addthresholdlist( IONotifyController* ic, SInfo& inf, IONotifyController::ThresholdExtList& lst, bool force=false );
		
		/*! регистрация дискретного датчика*/
		static inline void dsRegistration( IONotifyController* ic, IOController::UniDigitalIOInfo& inf, bool force=false )
		{
			ic->dsRegistration(inf,force);
		}

		/*! регистрация аналогового датчика*/
		static inline void asRegistration( IONotifyController* ic, IOController::UniAnalogIOInfo& inf, bool force=false )
		{
			ic->asRegistration(inf,force);
		}

		static inline IOController::AIOStateList::iterator aioFind(IONotifyController* ic, UniSetTypes::KeyType k)
		{
			return ic->myafind(k);
		}

		static inline IOController::DIOStateList::iterator dioFind(IONotifyController* ic, UniSetTypes::KeyType k)
		{
			return ic->mydfind(k);
		}

		static inline IOController::DIOStateList::iterator dioEnd( IONotifyController* ic )
		{
			return ic->mydioEnd();
		}
		static inline IOController::AIOStateList::iterator aioEnd( IONotifyController* ic )
		{
			return ic->myaioEnd();
		}
		static inline IOController::DIOStateList::iterator dioBegin( IONotifyController* ic )
		{
			return ic->mydioBegin();
		}
		static inline IOController::AIOStateList::iterator aioBegin( IONotifyController* ic )
		{
			return ic->myaioBegin();
		}
		
};
// ------------------------------------------------------------------------------------------
/*!
 * \brief Реализация сохранения списка заказчиков в xml(работа с файлом проекта)
 *	Реализует сохранение списка заказчиков в xml-файле (версия для работы с файлом проекта).
*/ 
class NCRestorer_XML:
	public Restorer_XML,
	public NCRestorer
{
	public:

		/*!
			\param fname - файл. (формата uniset-project)
		*/	
		NCRestorer_XML(const std::string fname);

		/*!
			\param fname - файл. (формата uniset-project)
			\param sensor_filterField - читать из списка только те узлы, у которых filterField="filterValue"
			\param sensor_filterValue - значение для фильтрования списка
		*/	
		NCRestorer_XML( const std::string fname, const std::string sensor_filterField, const std::string sensor_filterValue="" );

	    virtual ~NCRestorer_XML();
		NCRestorer_XML();

		/*! Установить фильтр на чтение списка 'зависимостей')
			\note Функцию необходимо вызывать до вызова buildDependsList(...)
		 */
		void setDependsFilter( const std::string filterField, const std::string filterValue="" );

		bool setFileName( const std::string& file, bool create );
		inline std::string getFileName(){ return fname; }

		/*! установить функцию для callback-вызова 
			при чтении списка пороговых датчиков. 

			bool xxxMyClass::myfunc(UniXML& xml, 
									UniXML_iterator& it, xmlNode* sec)
			uxml	- интерфейс для работы с xml-файлом
			it 	- интератор(указатель) на текущий считываемый xml-узел (<sensor>)
			sec	- указатель на корневой узел секции (<threshold>)
		*/
		void setReadThresholdItem( ReaderSlot sl );

		/*! установить функцию для callback-вызова при чтении списка зависимостей. 

			bool xxxMyClass::myfunc(UniXML& xml, 
									UniXML_iterator& it, xmlNode* sec)
			uxml	- интерфейс для работы с xml-файлом
			it 	- интератор(указатель) на текущий считываемый xml-узел (<sensor>)
			sec	- указатель на корневой узел секции (<depend>)
		*/
		void setReadDependItem( ReaderSlot sl );


		typedef sigc::slot<bool,UniXML&,UniXML_iterator&,xmlNode*,SInfo&> NCReaderSlot;

		void setNCReadItem( NCReaderSlot sl );

		virtual void read(IONotifyController* ic, const std::string filename="" );
		virtual void read(IONotifyController* ic, UniXML& xml );

		virtual void dump(IONotifyController* ic, SInfo& inf, const IONotifyController::ConsumerList& lst);
		virtual void dumpThreshold(IONotifyController* ic, SInfo& inf, const IONotifyController::ThresholdExtList& lst);

		virtual void buildDependsList( IONotifyController* ic, const std::string fn="" );
		virtual void buildDependsList( IONotifyController* ic, UniXML& xml );
		
	protected:
		
		bool check_thresholds_item( UniXML_iterator& it );
		bool check_depend_item( UniXML_iterator& it );
		void read_consumers(UniXML& xml, xmlNode* node, NCRestorer_XML::SInfo& inf, IONotifyController* ic );
		void read_list(UniXML& xml, xmlNode* node, IONotifyController* ic);
		void read_thresholds(UniXML& xml, xmlNode* node, IONotifyController* ic);
		void build_depends( UniXML& xml, xmlNode* node, IONotifyController* ic );
		void init( std::string fname );

		bool getBaseInfo( UniXML& xml, xmlNode* it, IOController_i::SensorInfo& si );
		bool getSensorInfo(UniXML& xml, xmlNode* snode, SInfo& si );
		bool getConsumerList(UniXML& xml,xmlNode* node, IONotifyController::ConsumerList& lst);
		bool getThresholdInfo(UniXML& xml,xmlNode* tnode, IONotifyController::ThresholdInfoExt& ti);
		bool getDependsInfo( UniXML& xml, xmlNode* node, IOController::DependsInfo& di );

		static void set_dumptime( UniXML& xml, xmlNode* node );
		static xmlNode* bind_node(UniXML& xml, xmlNode* root, const std::string& nodename, const std::string nm="");
		static xmlNode* rebind_node(UniXML& xml, xmlNode* root, const std::string& nodename, const std::string nm="");		


		std::string s_filterField;
		std::string s_filterValue;
		std::string c_filterField;
		std::string c_filterValue;
		std::string d_filterField;
		std::string d_filterValue;

		std::string fname;
		UniXML uxml;
		ReaderSlot rtslot;
		ReaderSlot depslot;
		NCReaderSlot ncrslot;

	private:
};
// ------------------------------------------------------------------------------------------
#endif
