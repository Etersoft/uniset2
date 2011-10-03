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
 *  \author Pavel Vainerman
*/
// -------------------------------------------------------------------------- 
#include <sstream>
#include "Debug.h"
#include "Configuration.h"
#include "NCRestorer.h"
#include "Exceptions.h"
#include "ORepHelpers.h"
// ------------------------------------------------------------------------------------------
using namespace std;
using namespace UniversalIO;
using namespace UniSetTypes;
// ------------------------------------------------------------------------------------------
NCRestorer_XML::NCRestorer_XML( const string fname ):
s_filterField(""),
s_filterValue(""),
c_filterField(""),
c_filterValue(""),
d_filterField(""),
d_filterValue(""),
t_filterField(""),
t_filterValue(""),
fname(fname)
{
	init(fname);
}

NCRestorer_XML::NCRestorer_XML(const string fname, 
									const std::string f_field, 
									const std::string f_value):
s_filterField(f_field),
s_filterValue(f_value),
c_filterField(""),
c_filterValue(""),
d_filterField(""),
d_filterValue(""),
t_filterField(""),
t_filterValue(""),
fname(fname)
{
	init(fname);
	setItemFilter(f_field,f_value);
}

NCRestorer_XML::NCRestorer_XML():
fname("")
{
}

// ------------------------------------------------------------------------------------------
NCRestorer_XML::~NCRestorer_XML()
{
	uxml.close();
}
// ------------------------------------------------------------------------------------------
void NCRestorer_XML::init( std::string fname )
{
	/*! 
		\warning Файл открывается только при создании... 
		Т.е. не будут учтены изменения в промежутке между записью(dump-а) файла
	*/
	try
	{
		uxml.open(fname);
	}
	catch(UniSetTypes::NameNotFound& ex)
	{
		unideb[Debug::WARN] << "(NCRestorer_XML): файл " << fname << " не найден, создаём новый...\n";
	}
}
// ------------------------------------------------------------------------------------------
void NCRestorer_XML::dump(IONotifyController* ic, SInfo& inf, 
					const IONotifyController::ConsumerList& lst)
{
	if( unideb.debugging(Debug::WARN) )	
		unideb[Debug::WARN] << "NCRestorer_XML::dump NOT SUPPORT!!!!" << endl;
}
// ------------------------------------------------------------------------------------------
void NCRestorer_XML::dumpThreshold(IONotifyController* ic, SInfo& inf, 
					const IONotifyController::ThresholdExtList& lst)
{
	if( unideb.debugging(Debug::WARN) )	
		unideb[Debug::WARN] << "NCRestorer_XML::dumpThreshold NOT SUPPORT!!!!" << endl;
}
// ------------------------------------------------------------------------------------------
void NCRestorer_XML::read_list( UniXML& xml, xmlNode* node, IONotifyController* ic )
{
	UniXML_iterator it(node);
	if( !it.goChildren() )
		return;

	for( ;it.getCurrent(); it.goNext() )
	{
		if( !check_list_item(it) )
			continue;
		
		NCRestorer_XML::SInfo inf;
	
		if( !getSensorInfo(xml,it,inf) )
		{
			unideb[Debug::WARN] << ic->getName() << "(read_list): не смог получить информацию по датчику " << endl;
			continue;
		}
		
		inf.undefined = false;

		// т.к. в функции может обновится inf
		// вызываем перед регистрацией
		// (потому-что в xxRegistration inf будет скопирована
		ncrslot(xml,it,node,inf);

		switch(inf.type)
		{
			case UniversalIO::DigitalOutput:
			case UniversalIO::DigitalInput:
			{
				try
				{	
					IOController::UniDigitalIOInfo dinf(inf);
					dinf.real_state = dinf.state;
					dsRegistration(ic,dinf,true);
				}
				catch(Exception& ex)
				{
					unideb[Debug::WARN] << "(read_list): " << ex << endl;
				}
			}
			break;

			case UniversalIO::AnalogOutput:
			case UniversalIO::AnalogInput:
			{
				try
				{
					asRegistration(ic, inf, true);
				}
				catch(Exception& ex)
				{
					unideb[Debug::WARN] << "(read_list): " << ex << endl;
				}
			}
			break;
			
			default:
				break;
		}
		
		rslot(xml,it,node);
		read_consumers(xml,it,inf,ic);
	}
}
// ------------------------------------------------------------------------------------------
void NCRestorer_XML::read( IONotifyController* ic, const string fn )
{
	UniXML* confxml = conf->getConfXML();

	if( !fn.empty() )
	{
		 // оптимизация (не загружаем второй раз xml-файл)
		if( fn == conf->getConfFileName() && confxml )
			read( ic, *confxml );
		else
		{
			UniXML xml(fn);
			read( ic, xml );
			xml.close();
		}
	}
	else if( !fname.empty() )
	{
		// оптимизация (не загружаем второй раз xml-файл)
		if( fname == conf->getConfFileName() && confxml )
			read( ic, *confxml );
		else
		{
			uxml.close();
			uxml.open(fname);
			read(ic,uxml);
		}
	}
}
// ------------------------------------------------------------------------------------------
void NCRestorer_XML::read( IONotifyController* ic, UniXML& xml )
{
	xmlNode* node;
	
	if( (&xml) == conf->getConfXML() )
		node = conf->getXMLSensorsSection();
	else
		node = xml.findNode(xml.getFirstNode(),"sensors");

	if( node )
		read_list(xml, node, ic);

	xmlNode* tnode( xml.findNode(xml.getFirstNode(),"thresholds") );
	if( tnode )
		read_thresholds(xml, tnode, ic);

}
// ------------------------------------------------------------------------------------------
bool NCRestorer_XML::getBaseInfo( UniXML& xml, xmlNode* it, IOController_i::SensorInfo& si )
{
	string sname( xml.getProp(it,"name"));
	if( sname.empty() )
	{
		unideb[Debug::WARN] << "(getSensorInfo): не указано имя датчика... пропускаем..." << endl;
		return false;
	}

	// преобразуем в полное имя
	ObjectId sid = UniSetTypes::DefaultObjectId;

	string id(xml.getProp(it,"id"));
	if( !id.empty() )
		sid = uni_atoi( id );
	else
		sid = conf->getSensorID(sname);

	if( sid == UniSetTypes::DefaultObjectId )
	{
		unideb[Debug::CRIT] << "(getSensorInfo): НЕ НАЙДЕН ИДЕНТИФИКАТОР датчика --> " << sname << endl;
		return false;
	}
	
	ObjectId snode = conf->getLocalNode();
	string snodename(xml.getProp(it,"node"));
	if( !snodename.empty() )
		snode = conf->getNodeID(snodename);

	if( snode == UniSetTypes::DefaultObjectId )
	{
		unideb[Debug::CRIT] << "(getSensorInfo): НЕ НАЙДЕН ИДЕНТИФИКАТОР узла --> " << snodename << endl;
		return false;
	}

	si.id =	sid; 
	si.node = snode;
	return true;	
}
// ------------------------------------------------------------------------------------------
bool NCRestorer_XML::getSensorInfo( UniXML& xml, xmlNode* it, SInfo& inf )
{
	if( !getBaseInfo(xml,it,inf.si) )
		return false;

	inf.priority = Message::Medium;
	string prior(xml.getProp(it,"priority"));
	if( prior == "Low" )
		inf.priority = Message::Low;
	else if( prior == "Medium" )
		inf.priority = Message::Medium;
	else if( prior == "High" )
		inf.priority = Message::High;
	else if( prior == "Super" )
		inf.priority = Message::Super;
	else
	{
		inf.priority = Message::Medium;
		if( unideb.debugging(Debug::INFO) )
		{
			unideb[Debug::INFO] << "(NCRestorer_XML:getSensorInfo): не указан приоритет для "
				<< xml.getProp(it,"name") << endl;
		}
	}


	inf.type = UniSetTypes::getIOType(xml.getProp(it,"iotype"));
	if( inf.type == UniversalIO::UnknownIOType )
	{
		unideb[Debug::CRIT] << "(NCRestorer_XML:getSensorInfo): unknown iotype=" << xml.getProp(it,"iotype")
			<< " for  " << xml.getProp(it,"name") << endl;
		return false;
	}

	// калибровка
	if( inf.type == UniversalIO::AnalogInput || inf.type == UniversalIO::AnalogOutput )
	{
		inf.ci.minRaw = xml.getIntProp(it,"rmin");
		inf.ci.maxRaw = xml.getIntProp(it,"rmax");
		inf.ci.minCal = xml.getIntProp(it,"cmin");
		inf.ci.maxCal = xml.getIntProp(it,"cmax");
		inf.ci.sensibility = xml.getIntProp(it,"sensibility");
		inf.ci.precision = xml.getIntProp(it,"precision");
	}
	else
	{
		inf.ci.minRaw = 0;
		inf.ci.maxRaw = 0;
		inf.ci.minCal = 0;
		inf.ci.maxCal = 0;
		inf.ci.sensibility = 0;
		inf.ci.precision = 0;
	}

	inf.default_val = xml.getIntProp(it,"default");
	inf.db_ignore = xml.getIntProp(it,"db_ignore");
	inf.value 		= inf.default_val;
	inf.undefined = false;
	inf.real_value = inf.value;
	return true;
}
// ------------------------------------------------------------------------------------------
void NCRestorer_XML::read_thresholds(UniXML& xml, xmlNode* node, IONotifyController* ic )
{
	UniXML_iterator it(node);
	if( !it.goChildren() )
		return;

	for( ;it.getCurrent(); it.goNext() )
	{
		if( !check_thresholds_item(it) )
			continue;
		
		NCRestorer_XML::SInfo inf;
		if( !getSensorInfo(xml,it.getCurrent(),inf) )
		{
			unideb[Debug::WARN] << ic->getName() 
				<< "(read_thresholds): не смог получить информацию по датчику" << endl;
			continue;
		}

		if( unideb.debugging(Debug::INFO) )	
		{
			unideb[Debug::INFO] << ic->getName() << "(read_thresholds): " 
				<< it.getProp("name") << endl;
			// conf->oind->getNameById(inf.si.id,inf.si.node) << endl;
		}
	
		UniXML_iterator tit(it);
		if( !tit.goChildren() )
			continue;

		IONotifyController::ThresholdExtList tlst;
		for( ;tit; tit.goNext() )
		{
			IONotifyController::ThresholdInfoExt ti(0,0,0,0);
			if( !getThresholdInfo(xml,tit,ti) )
			{
				unideb[Debug::WARN] << ic->getName() 
							<< "(read_thresholds): не смог получить информацию о пороге"
							<< " для датчика "
							<< conf->oind->getNameById(inf.si.id,inf.si.node) << endl;
				continue;
			}

			if( unideb.debugging(Debug::INFO) )
			{
				unideb[Debug::INFO] << "(read_thresholds): \tthreshold low=" 
							<< ti.lowlimit << " \thi=" << ti.hilimit
							<< " \t sb=" << ti.sensibility 
							<< " \t sid=" << ti.sid 
							<< " \t inverse=" << ti.inverse 
							<< endl << flush;
			}

			xmlNode* cnode = find_node(xml,tit,"consumers","");
			if( cnode )
			{
				UniXML_iterator ask_it(cnode);
				if( ask_it.goChildren() )
				{
					if( !getConsumerList(xml,ask_it,ti.clst) )
					{
						unideb[Debug::WARN] << ic->getName() 
								<< "(read_thresholds): не смог получить список заказчиков"
								<< " для порога " << ti.id 
								<< " датчика " << conf->oind->getNameById(inf.si.id,inf.si.node) << endl;
					}
				}
			}
		
			// порог добавляем в любом случае, даже если список заказчиков пуст...
			tlst.push_back(ti);
			rtslot(xml,tit,it);
		}

		addthresholdlist(ic,inf,tlst);
	}
}
// ------------------------------------------------------------------------------------------

void NCRestorer_XML::read_consumers( UniXML& xml, xmlNode* it, 
										NCRestorer_XML::SInfo& inf, IONotifyController* ic )
{
	// в новых ask-файлах список выделен <consumers>...</consumers>,
	xmlNode* cnode = find_node(xml,it,"consumers","");
	if( cnode )
	{
		UniXML_iterator cit(cnode);
		if( cit.goChildren() )
		{
			IONotifyController::ConsumerList lst;
			if( getConsumerList(xml,cit,lst) )
				addlist(ic,inf,lst,true);
		}
	}
}
// ------------------------------------------------------------------------------------------

bool NCRestorer_XML::getConsumerList( UniXML& xml,xmlNode* node, 
										IONotifyController::ConsumerList& lst )
{
	UniXML_iterator it(node);
	for(;it;it.goNext())
	{
		if( !check_consumer_item(it) )
			continue;

		ConsumerInfo ci;
		if( !getConsumerInfo(it,ci.id,ci.node) )
			continue;

		IONotifyController::ConsumerInfoExt cinf(ci);
		lst.push_back(cinf);
		cslot(xml,it,node);
	}

	return true;

}
// ------------------------------------------------------------------------------------------
bool NCRestorer_XML::getThresholdInfo( UniXML& xml,xmlNode* node, 
										IONotifyController::ThresholdInfoExt& ti )
{
	UniXML_iterator uit(node);

	string sid_name = uit.getProp("sid");
	if( !sid_name.empty() )
	{
		ti.sid = conf->getSensorID(sid_name);
		if( ti.sid == UniSetTypes::DefaultObjectId )
		{
			unideb[Debug::CRIT] << "(NCRestorer_XML:getThresholdInfo): " 
					<< " Not found ID for " << sid_name << endl;
		}
		else
		{
			UniversalIO::IOTypes iotype = conf->getIOType(ti.sid);
			// Пока что IONotifyController поддерживает работу только с 'DI'.
			if( iotype != UniversalIO::DigitalInput )
			{
				 unideb[Debug::CRIT] << "(NCRestorer_XML:getThresholdInfo): "
					<< " Bad iotype(" << iotype << ") for " << sid_name << ". iotype must be 'DI'!" << endl;
				return false;
			}

		}
	}

	ti.id 			= uit.getIntProp("id");
	ti.lowlimit 	= uit.getIntProp("lowlimit");
	ti.hilimit 		= uit.getIntProp("hilimit");
	ti.sensibility 	= uit.getIntProp("sensibility");
	ti.inverse 		= uit.getIntProp("inverse");
	ti.state 		= IONotifyController_i::NormalThreshold;
	return true;
}
// ------------------------------------------------------------------------------------------
bool NCRestorer_XML::check_thresholds_item( UniXML_iterator& it )
{	
	return UniSetTypes::check_filter(it,t_filterField,t_filterValue);
}
// ------------------------------------------------------------------------------------------
void NCRestorer_XML::setReadThresholdItem( ReaderSlot sl )
{
	rtslot = sl;
}
// ------------------------------------------------------------------------------------------
void NCRestorer_XML::setReadDependItem( ReaderSlot sl )
{
	depslot = sl;
}
// ------------------------------------------------------------------------------------------
void NCRestorer_XML::setDependsFilter( const std::string field, const std::string val )
{
	d_filterField = field;
	d_filterValue = val;
}
// -----------------------------------------------------------------------------
void NCRestorer_XML::setThresholdsFilter( const std::string field, const std::string val )
{
	t_filterField = field;
	t_filterValue = val;
}
// -----------------------------------------------------------------------------
void NCRestorer_XML::setNCReadItem( NCReaderSlot sl )
{
	ncrslot = sl;
}
// -----------------------------------------------------------------------------
void NCRestorer_XML::buildDependsList( IONotifyController* ic, const std::string fn )
{
	UniXML* confxml = conf->getConfXML();
	if( !fn.empty() )
	{
		 // оптимизация (не загружаем второй раз xml-файл)
		if( fn == conf->getConfFileName() && confxml )
			buildDependsList( ic, *confxml );
		else
		{
			UniXML xml(fn);
			buildDependsList( ic, xml );
			xml.close();
		}
	}
	else if( !fname.empty() )
	{
		// оптимизация (не загружаем второй раз xml-файл)
		if( fname == conf->getConfFileName() && confxml )
			buildDependsList( ic, *confxml );
		else
		{
			uxml.close();
			uxml.open(fname);
			buildDependsList(ic,uxml);
		}
	}
	else if( confxml )
	{
		buildDependsList( ic, *confxml );
	}
	else
	{
		unideb[Debug::CRIT] << "(NCRestorer_XML:buildDependsList): configure xml-file not defined..." << endl;
	}
}

// -----------------------------------------------------------------------------
void NCRestorer_XML::buildDependsList( IONotifyController* ic, UniXML& xml )
{
	xmlNode* node;
	
	if( (&xml) == conf->getConfXML() )
		node = conf->getXMLSensorsSection();
	else
		node = xml.findNode(xml.getFirstNode(),"sensors");

	if( node )
		build_depends(xml, node, ic);
}
// -----------------------------------------------------------------------------
void NCRestorer_XML::build_depends( UniXML& xml, xmlNode* node, IONotifyController* ic )
{
	UniXML_iterator it(node);
	if( !it.goChildren() )
		return;

	for( ;it.getCurrent(); it.goNext() )
	{
		if( !check_list_item(it) )
			continue;

		xmlNode* dnode = find_node(xml,it,"depends","");
		if( !dnode )
			continue;

		IOController::DependsInfo mydepinfo;
		if( !getBaseInfo(xml,it,mydepinfo.si) )
			continue;
		
		UniSetTypes::KeyType k = UniSetTypes::key(mydepinfo.si.id,mydepinfo.si.node);
		mydepinfo.dit = dioFind(ic,k);
		mydepinfo.ait = aioFind(ic,k);
		if( mydepinfo.dit==dioEnd(ic) && mydepinfo.ait==aioEnd(ic) )
		{
			unideb[Debug::CRIT] << "(NCRestorer_XML:build_depends): Датчик " 
				<< xml.getProp(node,"name")
				<< " НЕ НАЙДЕН ВО ВНУТРЕННЕМ СПИСКЕ! " << endl;
				continue;
		}		

		UniXML_iterator dit(dnode);
		if( dit.goChildren() )
		{
			for( ;dit.getCurrent(); dit.goNext() )
			{
				if( !check_depend_item(dit) )
					continue;
			
				IOController::DependsInfo blk; // информации о блокировщике данного датчика
				if( getDependsInfo(xml,dit,blk) )
				{	
					k = UniSetTypes::key(blk.si.id,blk.si.node);
					blk.dit = dioFind(ic,k);
					blk.ait = aioFind(ic,k);
					if( blk.dit==dioEnd(ic) && blk.ait==aioEnd(ic) )
					{
						unideb[Debug::CRIT] << ic->getName() << "(NCRestorer_XML:build_depends): " 
							<< " Не найдена зависимость на " << xml.getProp(dit,"name")
							<< " для " << xml.getProp(node,"name") << endl;
						continue;
					}
							
					mydepinfo.block_invert = dit.getIntProp("block_invert");
					long block_val = dit.getIntProp("block_value");

					long defval 	= 0;
					if( blk.dit != dioEnd(ic) )
						defval = blk.dit->second.default_val;
					else if( blk.ait != aioEnd(ic) )
						defval = blk.ait->second.default_val;

					// Проверка начальных условий для высталения блокировки
					bool blk_set = defval ? false : true;
					if( mydepinfo.block_invert )
						blk_set ^= true;

					if( mydepinfo.dit!=dioEnd(ic) )
					{
						mydepinfo.dit->second.blocked = blk_set;
						mydepinfo.dit->second.block_state = (bool)block_val;
						mydepinfo.dit->second.state = defval;
						mydepinfo.dit->second.real_state = defval;
						if( blk_set )
							mydepinfo.dit->second.state = (bool)block_val;
					}
					else if( mydepinfo.ait!=aioEnd(ic) )
					{
						mydepinfo.ait->second.blocked = blk_set;
						mydepinfo.ait->second.block_value = block_val;
						if( blk_set )
						{
							mydepinfo.ait->second.real_value = mydepinfo.ait->second.value;
							mydepinfo.ait->second.value = block_val;
						}
					}

					// вносим 'себя' в список зависимостей для указанного датчика
					// (без проверки на дублирование
					// т.к. не может быть два одинаковых ID 
					// в конф. файле...
					if( blk.dit != dioEnd(ic) )
					{
						blk.dit->second.dlst.push_back(mydepinfo);
						if( unideb.debugging(Debug::INFO) )	
						{
							unideb[Debug::INFO] << ic->getName() << "(NCRestorer_XML:build_depends):" 
								<< " add " << xml.getProp(it,"name") 
								<< " to list of depends for " << xml.getProp(dit,"name")
								<< " blk_set=" << blk_set
								<< endl;
						}
					}
					else if( blk.ait != aioEnd(ic) )
					{
						blk.ait->second.dlst.push_back(mydepinfo);
						if( unideb.debugging(Debug::INFO) )	
						{
							unideb[Debug::INFO] << ic->getName() << "(NCRestorer_XML:build_depends):" 
								<< " add " << xml.getProp(it,"name") 
								<< " to list of depends for " << xml.getProp(dit,"name")
								<< " blk_set=" << blk_set
								<< endl;
						}
					}
					
					depslot(xml,dit,node);
				}
			}
		}
	}
}
// -----------------------------------------------------------------------------

bool NCRestorer_XML::check_depend_item( UniXML_iterator& it )
{
	return UniSetTypes::check_filter(it,d_filterField,d_filterValue);
}
// ------------------------------------------------------------------------------------------
bool NCRestorer_XML::getDependsInfo( UniXML& xml, xmlNode* it, IOController::DependsInfo& di )
{
	return getBaseInfo(xml,it,di.si);
}
// -----------------------------------------------------------------------------
