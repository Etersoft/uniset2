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
#include <sstream>
#include "Configuration.h"
#include "IOController.h"
#include "IONotifyController.h"
#include "IOConfig_XML.h"
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
namespace uniset
{
	// --------------------------------------------------------------------------
	IOConfig_XML::IOConfig_XML()
	{

	}
	// --------------------------------------------------------------------------
	IOConfig_XML::IOConfig_XML( const std::string& confile,
								const std::shared_ptr<Configuration>& _conf ):
		conf(_conf)
	{
		uxml = make_shared<UniXML>(confile);
	}
	// --------------------------------------------------------------------------
	IOConfig_XML::IOConfig_XML(const std::shared_ptr<UniXML>& _xml,
							   const std::shared_ptr<Configuration>& _conf,
							   xmlNode* _root ):
		conf(_conf),
		uxml(_xml),
		root(_root)
	{
	}
	// --------------------------------------------------------------------------
	IOConfig_XML::~IOConfig_XML()
	{
	}
	// --------------------------------------------------------------------------
	IOController::IOStateList IOConfig_XML::read()
	{
		IOController::IOStateList lst;

		if( !root )
			root = uxml->findNode( uxml->getFirstNode(), "sensors");

		if( !root )
			return lst;

		UniXML::iterator it(root);

		if( !it.goChildren() )
			return lst;

		lst = read_list(it);
		// только после чтения всех датчиков и формирования списка IOList
		// можно инициализировать списки зависимостей
		init_depends_signals(lst);

		xmlNode* tnode = uxml->findNode(uxml->getFirstNode(), "thresholds");

		if( tnode )
			init_thresholds(tnode, lst);

		return lst;
	}
	// --------------------------------------------------------------------------
	IOController::IOStateList IOConfig_XML::read_list( xmlNode* node )
	{
		IOController::IOStateList lst;

		UniXML::iterator it(node);

		if( !it.getCurrent() )
			return lst;

		for( ; it.getCurrent(); ++it )
		{
			if( !check_list_item(it) )
				continue;

			auto inf = make_shared<IOController::USensorInfo>();

			if( !getSensorInfo(it, inf) )
			{
				uwarn << "(IOConfig_XML::read_list): FAILED read parameters for " << it.getProp("name") << endl;
				continue;
			}

			inf->undefined = false;

			ncrslot(uxml, it, node, inf);
			rslot(uxml, it, node);
			//		read_consumers(xml, it, inf);

			lst.emplace( inf->si.id, std::move(inf) );
		}

		return lst;
	}
	// ------------------------------------------------------------------------------------------
	bool IOConfig_XML::getBaseInfo( xmlNode* node, IOController_i::SensorInfo& si ) const
	{
		UniXML::iterator it(node);
		string sname( it.getProp("name"));

		if( sname.empty() )
		{
			uwarn << "(IOConfig_XML::getBaseInfo): Unknown sensor name... skipped..." << endl;
			return false;
		}

		// преобразуем в полное имя
		ObjectId sid = uniset::DefaultObjectId;

		string id(it.getProp("id"));

		if( !id.empty() )
			sid = uni_atoi( id );
		else
			sid = conf->getSensorID(sname);

		if( sid == uniset::DefaultObjectId )
		{
			ostringstream err;
			err << "(IOConfig_XML::getBaseInfo): Not found ID for sensor --> " << sname;
			ucrit << err.str() << endl;
			throw SystemError(err.str());
		}

		ObjectId snode = conf->getLocalNode();
		string snodename(it.getProp("node"));

		if( !snodename.empty() )
			snode = conf->getNodeID(snodename);

		if( snode == uniset::DefaultObjectId )
		{
			ucrit << "(IOConfig_XML::getBaseInfo): Not found ID for node --> " << snodename << endl;
			return false;
		}

		si.id =    sid;
		si.node = snode;
		return true;
	}
	// ------------------------------------------------------------------------------------------
	bool IOConfig_XML::getSensorInfo( xmlNode* node,
									  std::shared_ptr<IOController::USensorInfo>& inf ) const
	{
		if( !getBaseInfo(node, inf->si) )
			return false;

		UniXML::iterator it(node);

		inf->priority = Message::Medium;
		string prior(it.getProp("priority"));

		if( prior == "Low" )
			inf->priority = Message::Low;
		else if( prior == "Medium" )
			inf->priority = Message::Medium;
		else if( prior == "High" )
			inf->priority = Message::High;
		else
			inf->priority = Message::Medium;

		inf->type = uniset::getIOType(it.getProp("iotype"));

		if( inf->type == UniversalIO::UnknownIOType )
		{
			ostringstream err;
			err << "(IOConfig_XML:getSensorInfo): unknown iotype=" << it.getProp("iotype")
				<< " for  " << it.getProp("name");
			ucrit << err.str() << endl;
			throw SystemError(err.str());
		}

		// калибровка
		if( inf->type == UniversalIO::AI || inf->type == UniversalIO::AO )
		{
			inf->ci.minRaw = it.getIntProp("rmin");
			inf->ci.maxRaw = it.getIntProp("rmax");
			inf->ci.minCal = it.getIntProp("cmin");
			inf->ci.maxCal = it.getIntProp("cmax");
			inf->ci.precision = it.getIntProp("precision");
		}
		else
		{
			inf->ci.minRaw = 0;
			inf->ci.maxRaw = 0;
			inf->ci.minCal = 0;
			inf->ci.maxCal = 0;
			inf->ci.precision = 0;
		}

		inf->default_val = it.getIntProp("default");
		inf->dbignore = it.getIntProp("dbignore");
		inf->value = inf->default_val;
		inf->undefined = false;
		inf->real_value = inf->value;

		if( !it.getProp("undefined_value").empty() )
			inf->undef_value = it.getIntProp("undefined_value");

		string d_txt( it.getProp("depend") );

		if( !d_txt.empty() )
		{
			inf->d_si.id = conf->getSensorID(d_txt);

			if( inf->d_si.id == uniset::DefaultObjectId )
			{
				ostringstream err;
				err << "(IOConfig_XML::getSensorInfo): sensor='"
					<< it.getProp("name") << "' err: "
					<< " Unknown SensorID for depend='"  << d_txt;

				ucrit << err.str() << endl;
				throw SystemError(err.str());
			}

			inf->d_si.node = conf->getLocalNode();

			// по умолчанию срабатывание на "1"
			inf->d_value = it.getProp("depend_value").empty() ? 1 : it.getIntProp("depend_value");
			inf->d_off_value = it.getPIntProp("depend_off_value", 0);
		}

		return true;
	}
	// ------------------------------------------------------------------------------------------
	void IOConfig_XML::init_depends_signals( IOController::IOStateList& lst )
	{
		for( auto && it : lst )
		{
			// обновляем итераторы...
			it.second->d_usi = it.second;

			if( it.second->d_si.id == DefaultObjectId )
				continue;

			uinfo << "(IOConfig_XML::init_depends_signals): "
				  << " init depend: '" << conf->oind->getMapName(it.second->si.id) << "'"
				  << " dep_name=(" << it.second->d_si.id << ")'" << conf->oind->getMapName(it.second->d_si.id) << "'"
				  << endl;

			auto dit = lst.find(it.second->d_si.id);

			if( dit != lst.end() )
			{
				uniset_rwmutex_rlock lock(it.second->val_lock);
				dit->second->sigChange.connect( sigc::mem_fun( it.second.get(), &IOController::USensorInfo::checkDepend) );
			}
		}
	}
	// -----------------------------------------------------------------------------
	void IOConfig_XML::init_thresholds( xmlNode* node, IOController::IOStateList& iolist )
	{
		UniXML::iterator it(node);

		if( !it.goChildren() )
			return;

		for( ; it.getCurrent(); ++it )
		{
			if( !check_thresholds_item(it) )
				continue;

			IOController_i::SensorInfo si;

			if( !getBaseInfo(it, si) )
			{
				ostringstream err;
				err << "(IOConfig_XML::init_thresholds): failed read parameters for threashold " << it.getProp("name");
				ucrit << err.str() << endl;
				throw uniset::SystemError(err.str());
			}

			auto inf = iolist.find(si.id);

			if( inf == iolist.end() )
			{
				ostringstream err;
				err << "(IOConfig_XML::init_thresholds): NOT FOUND " << it.getProp("name");
				ucrit << err.str() << endl;
				throw uniset::SystemError(err.str());
			}

			ulog3 << "(IOConfig_XML::read_thresholds): " << it.getProp("name") << endl;

			UniXML::iterator tit(it);

			if( !tit.goChildren() )
				continue;

			IONotifyController::ThresholdExtList tlst;

			for( ; tit.getCurrent(); tit++ )
			{
				auto ti = make_shared<IONotifyController::UThresholdInfo>(0, 0, 0, false);

				if( !getThresholdInfo(tit, ti) )
				{
					ostringstream err;
					err << "(IOConfig_XML::read_thresholds): FAILED read threshold parameters for "
						<< conf->oind->getNameById(si.id);

					ucrit << err.str() << endl;
					throw uniset::SystemError(err.str());
				}

				ulog3  << "(IOConfig_XML::read_thresholds): \tthreshold low="
					   << ti->lowlimit << " \thi=" << ti->hilimit
					   << " \t sid=" << ti->sid
					   << " \t invert=" << ti->invert
					   << endl << flush;

				// начальная инициализация итератора
				ti->sit = iolist.end();

				// порог добавляем в любом случае, даже если список заказчиков пуст...
				tlst.emplace_back(ti);
				rtslot(uxml, tit, it);
			}

			std::swap(inf->second->thresholds, tlst);
		}
	}
	// ------------------------------------------------------------------------------------------
#if 0
	void IOConfig_XML::read_consumers( const std::shared_ptr<UniXML>& xml, xmlNode* it,
									   std::shared_ptr<IOConfig_XML::SInfo>& inf )
	{
		// в новых ask-файлах список выделен <consumers>...</consumers>,
		xmlNode* cnode = find_node(xml, it, "consumers", "");

		if( cnode )
		{
			UniXML::iterator cit(cnode);

			if( cit.goChildren() )
			{
				IONotifyController::ConsumerListInfo lst;

				if( getConsumerList(xml, cit, lst) )
				{
					std::shared_ptr<IOController::USensorInfo> uinf = std::static_pointer_cast<IOController::USensorInfo>(inf);
					addlist(ic, uinf, std::move(lst), true);
				}
			}
		}
	}
	// ------------------------------------------------------------------------------------------

	bool IOConfig_XML::getConsumerList( const std::shared_ptr<UniXML>& xml, xmlNode* node,
										IONotifyController::ConsumerListInfo& lst ) const
	{
		UniXML::iterator it(node);

		for(; it.getCurrent(); it++ )
		{
			if( !check_consumer_item(it) )
				continue;

			ConsumerInfo ci;

			if( !getConsumerInfo(it, ci.id, ci.node) )
				continue;

			lst.clst.emplace_back( std::move(ci) );
			cslot(xml, it, node);
		}

		return true;

	}
#endif
	// ------------------------------------------------------------------------------------------
	bool IOConfig_XML::getThresholdInfo( xmlNode* node, std::shared_ptr<IONotifyController::UThresholdInfo>& ti ) const
	{
		UniXML::iterator uit(node);

		string sid_name = uit.getProp("sid");

		if( !sid_name.empty() )
		{
			ti->sid = conf->getSensorID(sid_name);

			if( ti->sid == uniset::DefaultObjectId )
			{
				ostringstream err;
				err << "(IOConfig_XML:getThresholdInfo): "
					<< " Not found ID for " << sid_name;

				ucrit << err.str() << endl;
				throw uniset::SystemError(err.str());
			}
			else
			{
				UniversalIO::IOType iotype = conf->getIOType(ti->sid);

				// Пока что IONotifyController поддерживает работу только с 'DI/DO'.
				if( iotype != UniversalIO::DI && iotype != UniversalIO::DO )
				{
					ostringstream err;
					err << "(IOConfig_XML:getThresholdInfo): "
						<< " Bad iotype(" << iotype << ") for " << sid_name
						<< ". iotype must be 'DI' or 'DO'.";

					ucrit << err.str() << endl;
					throw uniset::SystemError(err.str());
				}
			}
		}

		ti->id           = uit.getIntProp("id");
		ti->lowlimit     = uit.getIntProp("lowlimit");
		ti->hilimit      = uit.getIntProp("hilimit");
		ti->invert       = uit.getIntProp("invert");
		ti->state        = IONotifyController_i::NormalThreshold;
		return true;
	}
	// ------------------------------------------------------------------------------------------
	bool IOConfig_XML::check_thresholds_item( UniXML::iterator& it ) const
	{
		return uniset::check_filter(it, t_filterField, t_filterValue);
	}
	// ------------------------------------------------------------------------------------------
	void IOConfig_XML::setReadThresholdItem( ReaderSlot sl )
	{
		rtslot = sl;
	}
	// ------------------------------------------------------------------------------------------
	void IOConfig_XML::setNCReadItem( NCReaderSlot sl )
	{
		ncrslot = sl;
	}
	// -----------------------------------------------------------------------------
	void IOConfig_XML::setThresholdsFilter( const std::string& field, const std::string& val )
	{
		t_filterField = field;
		t_filterValue = val;
	}
	// -----------------------------------------------------------------------------
	void IOConfig_XML::setItemFilter( const string& field, const string& val )
	{
		i_filterField = field;
		i_filterValue = val;
	}
	// -----------------------------------------------------------------------------
	void IOConfig_XML::setConsumerFilter( const string& field, const string& val )
	{
		c_filterField = field;
		c_filterValue = val;
	}
	// -----------------------------------------------------------------------------
	bool IOConfig_XML::getConsumerInfo( UniXML::iterator& it,
										ObjectId& cid, ObjectId& cnode ) const
	{
		if( !check_consumer_item(it) )
			return false;

		string cname( it.getProp("name"));

		if( cname.empty() )
		{
			uwarn << "(Restorer_XML:getConsumerInfo): не указано имя заказчика..." << endl;
			return false;
		}

		string otype(it.getProp("type"));

		if( otype == "controllers" )
			cname = uniset_conf()->getControllersSection() + "/" + cname;
		else if( otype == "objects" )
			cname = uniset_conf()->getObjectsSection() + "/" + cname;
		else if( otype == "services" )
			cname = uniset_conf()->getServicesSection() + "/" + cname;
		else
		{
			uwarn << "(Restorer_XML:getConsumerInfo): неизвестный тип объекта "
				  << otype << endl;
			return false;
		}

		cid = uniset_conf()->oind->getIdByName(cname);

		if( cid == uniset::DefaultObjectId )
		{
			ucrit << "(Restorer_XML:getConsumerInfo): НЕ НАЙДЕН ИДЕНТИФИКАТОР заказчика -->"
				  << cname << endl;
			return false;
		}

		string cnodename(it.getProp("node"));

		if( !cnodename.empty() )
			cnode = uniset_conf()->oind->getIdByName(cnodename);
		else
			cnode = uniset_conf()->getLocalNode();

		if( cnode == uniset::DefaultObjectId )
		{
			ucrit << "(Restorer_XML:getConsumerInfo): НЕ НАЙДЕН ИДЕНТИФИКАТОР узла -->"
				  << cnodename << endl;
			return false;
		}

		uinfo << "(Restorer_XML:getConsumerInfo): " << cname << ":" << cnodename << endl;
		return true;
	}
	// -----------------------------------------------------------------------------
	bool IOConfig_XML::check_list_item( UniXML::iterator& it ) const
	{
		return uniset::check_filter(it, i_filterField, i_filterValue);
	}
	// -----------------------------------------------------------------------------
	bool IOConfig_XML::check_consumer_item( UniXML::iterator& it ) const
	{
		return uniset::check_filter(it, c_filterField, c_filterValue);
	}
	// -----------------------------------------------------------------------------
	void IOConfig_XML::setReadItem( ReaderSlot sl )
	{
		rslot = sl;
	}
	// -----------------------------------------------------------------------------
	void IOConfig_XML::setReadConsumerItem( ReaderSlot sl )
	{
		cslot = sl;
	}
	// -----------------------------------------------------------------------------

} // end of uniset namespace
// --------------------------------------------------------------------------
