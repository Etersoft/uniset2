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
NCRestorer_XML::NCRestorer_XML( const string& fname ):
s_filterField(""),
s_filterValue(""),
c_filterField(""),
c_filterValue(""),
t_filterField(""),
t_filterValue(""),
fname(fname),
uxml(0)
{
    init(fname);
}

NCRestorer_XML::NCRestorer_XML(const string& fname, 
                                    const std::string& f_field, 
                                    const std::string& f_value):
s_filterField(f_field),
s_filterValue(f_value),
c_filterField(""),
c_filterValue(""),
t_filterField(""),
t_filterValue(""),
fname(fname),
uxml(0)
{
    init(fname);
    setItemFilter(f_field,f_value);
}

NCRestorer_XML::NCRestorer_XML():
fname(""),
uxml(0)
{
}

// ------------------------------------------------------------------------------------------
NCRestorer_XML::~NCRestorer_XML()
{
    if( uxml)
    {
        uxml->close();
        delete uxml;
    }
}
// ------------------------------------------------------------------------------------------
void NCRestorer_XML::init( const std::string& fname )
{
    /*! 
        \warning Файл открывается только при создании... 
        Т.е. не будут учтены изменения в промежутке между записью(dump-а) файла
    */
    try
    {
        if( fname == conf->getConfFileName() )
            uxml = const_cast<UniXML*>(conf->getConfXML());
        else
            uxml = new UniXML(fname);
    }
    catch( UniSetTypes::NameNotFound& ex )
    {
        uwarn << "(NCRestorer_XML): файл " << fname << " не найден, создаём новый...\n";
    }
}
// ------------------------------------------------------------------------------------------
void NCRestorer_XML::dump(const IONotifyController* ic, SInfo& inf, 
                    const IONotifyController::ConsumerListInfo& lst)
{
    uwarn << "NCRestorer_XML::dump NOT SUPPORT!!!!" << endl;
}
// ------------------------------------------------------------------------------------------
void NCRestorer_XML::dumpThreshold(const IONotifyController* ic, SInfo& inf, 
                    const IONotifyController::ThresholdExtList& lst)
{
    uwarn << "NCRestorer_XML::dumpThreshold NOT SUPPORT!!!!" << endl;
}
// ------------------------------------------------------------------------------------------
void NCRestorer_XML::read_list( const UniXML& xml, xmlNode* node, IONotifyController* ic )
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
            uwarn << ic->getName() << "(read_list): не смог получить информацию по датчику " << endl;
            continue;
        }

        inf.undefined = false;

        // т.к. в функции может обновится inf
        // вызываем перед регистрацией
        // (потому-что в xxRegistration inf будет скопирована
        ncrslot(xml,it,node,inf);

        switch(inf.type)
        {
            case UniversalIO::DO:
            case UniversalIO::DI:
            case UniversalIO::AO:
            case UniversalIO::AI:
            {
                try
                {
                    ioRegistration(ic, inf, true);
                }
                catch(Exception& ex)
                {
                    uwarn << "(read_list): " << ex << endl;
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
void NCRestorer_XML::read( IONotifyController* ic, const string& fn )
{
    const UniXML* confxml = conf->getConfXML();

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
        else if( uxml && uxml->isOpen() && uxml->filename == fn )
            read(ic,*uxml);
        else
        {
            uxml->close();
            uxml->open(fname);
            read(ic,*uxml);
        }
    }
}
// ------------------------------------------------------------------------------------------
void NCRestorer_XML::read( IONotifyController* ic, const UniXML& xml )
{
    xmlNode* node;

    if( (&xml) == conf->getConfXML() )
        node = conf->getXMLSensorsSection();
    else
        node = xml.findNode( xml.getFirstNode(),"sensors");

    if( node )
    {
        read_list(xml, node, ic);
        // только после чтения всех датчиков и формирования списка IOList
        // можно инициализировать списки зависимостей
        init_depends_signals(ic);
    }

    xmlNode* tnode( xml.findNode(xml.getFirstNode(),"thresholds") );
    if( tnode )
        read_thresholds(xml, tnode, ic);

}
// ------------------------------------------------------------------------------------------
bool NCRestorer_XML::getBaseInfo( const UniXML& xml, xmlNode* it, IOController_i::SensorInfo& si )
{
    string sname( xml.getProp(it,"name"));
    if( sname.empty() )
    {
        uwarn << "(getBaseInfo): не указано имя датчика... пропускаем..." << endl;
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
        ucrit << "(getBaseInfo): НЕ НАЙДЕН ИДЕНТИФИКАТОР датчика --> " << sname << endl;
        return false;
    }
    
    ObjectId snode = conf->getLocalNode();
    string snodename(xml.getProp(it,"node"));
    if( !snodename.empty() )
        snode = conf->getNodeID(snodename);

    if( snode == UniSetTypes::DefaultObjectId )
    {
        ucrit << "(getBaseInfo): НЕ НАЙДЕН ИДЕНТИФИКАТОР узла --> " << snodename << endl;
        return false;
    }

    si.id =    sid; 
    si.node = snode;
    return true;    
}
// ------------------------------------------------------------------------------------------
bool NCRestorer_XML::getSensorInfo( const UniXML& xml, xmlNode* it, SInfo& inf )
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
        inf.priority = Message::Medium;

    inf.type = UniSetTypes::getIOType(xml.getProp(it,"iotype"));
    if( inf.type == UniversalIO::UnknownIOType )
    {
        ucrit << "(NCRestorer_XML:getSensorInfo): unknown iotype=" << xml.getProp(it,"iotype")
            << " for  " << xml.getProp(it,"name") << endl;
        return false;
    }

    // калибровка
    if( inf.type == UniversalIO::AI || inf.type == UniversalIO::AO )
    {
        inf.ci.minRaw = xml.getIntProp(it,"rmin");
        inf.ci.maxRaw = xml.getIntProp(it,"rmax");
        inf.ci.minCal = xml.getIntProp(it,"cmin");
        inf.ci.maxCal = xml.getIntProp(it,"cmax");
        inf.ci.precision = xml.getIntProp(it,"precision");
    }
    else
    {
        inf.ci.minRaw = 0;
        inf.ci.maxRaw = 0;
        inf.ci.minCal = 0;
        inf.ci.maxCal = 0;
        inf.ci.precision = 0;
    }

    inf.default_val = xml.getIntProp(it,"default");
    inf.dbignore = xml.getIntProp(it,"dbignore");
    inf.value = inf.default_val;
    inf.undefined = false;
    inf.real_value = inf.value;

    string d_txt( xml.getProp(it, "depend") );
    if( !d_txt.empty() )
    {
        inf.d_si.id = conf->getSensorID(d_txt);
        if( inf.d_si.id == UniSetTypes::DefaultObjectId )
        {
            ucrit << "(NCRestorer_XML:getSensorInfo): sensor='" 
                    << xml.getProp(it,"name") << "' err: "
                    << " Unknown SensorID for depend='"  << d_txt
                    << endl;
            return false;
        }

        inf.d_si.node = conf->getLocalNode();

        // по умолчанию срабатывание на "1"
        inf.d_value = xml.getProp(it,"depend_value").empty() ? 1 : xml.getIntProp(it,"depend_value");
        inf.d_off_value = xml.getPIntProp(it,"depend_off_value",0);
    }

    return true;
}
// ------------------------------------------------------------------------------------------
void NCRestorer_XML::read_thresholds(const UniXML& xml, xmlNode* node, IONotifyController* ic )
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
            uwarn << ic->getName() 
                << "(read_thresholds): не смог получить информацию по датчику" << endl;
            continue;
        }

         uinfo << ic->getName() << "(read_thresholds): " << it.getProp("name") << endl;

        UniXML_iterator tit(it);
        if( !tit.goChildren() )
            continue;

        IONotifyController::ThresholdExtList tlst;
        for( ;tit; tit.goNext() )
        {
            IONotifyController::ThresholdInfoExt ti(0,0,0,0);
            if( !getThresholdInfo(xml,tit,ti) )
            {
                uwarn << ic->getName() 
                            << "(read_thresholds): не смог получить информацию о пороге"
                            << " для датчика "
                            << conf->oind->getNameById(inf.si.id,inf.si.node) << endl;
                continue;
            }

            uinfo  << "(read_thresholds): \tthreshold low=" 
                     << ti.lowlimit << " \thi=" << ti.hilimit
                     << " \t sid=" << ti.sid 
                     << " \t invert=" << ti.invert 
                     << endl << flush;

            xmlNode* cnode = find_node(xml,tit,"consumers","");
            if( cnode )
            {
                UniXML_iterator ask_it(cnode);
                if( ask_it.goChildren() )
                {
                    if( !getConsumerList(xml,ask_it,ti.clst) )
                    {
                        uwarn << ic->getName() 
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

void NCRestorer_XML::read_consumers( const UniXML& xml, xmlNode* it, 
                                        NCRestorer_XML::SInfo& inf, IONotifyController* ic )
{
    // в новых ask-файлах список выделен <consumers>...</consumers>,
    xmlNode* cnode = find_node(xml,it,"consumers","");
    if( cnode )
    {
        UniXML_iterator cit(cnode);
        if( cit.goChildren() )
        {
            IONotifyController::ConsumerListInfo lst;
            if( getConsumerList(xml,cit,lst) )
                addlist(ic,inf,lst,true);
        }
    }
}
// ------------------------------------------------------------------------------------------

bool NCRestorer_XML::getConsumerList( const UniXML& xml, xmlNode* node, 
                                        IONotifyController::ConsumerListInfo& lst )
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
        lst.clst.push_back(cinf);
        cslot(xml,it,node);
    }

    return true;

}
// ------------------------------------------------------------------------------------------
bool NCRestorer_XML::getThresholdInfo( const UniXML& xml,xmlNode* node, 
                                        IONotifyController::ThresholdInfoExt& ti )
{
    UniXML_iterator uit(node);

    string sid_name = uit.getProp("sid");
    if( !sid_name.empty() )
    {
        ti.sid = conf->getSensorID(sid_name);
        if( ti.sid == UniSetTypes::DefaultObjectId )
        {
            ucrit << "(NCRestorer_XML:getThresholdInfo): " 
                    << " Not found ID for " << sid_name << endl;
        }
        else
        {
            UniversalIO::IOType iotype = conf->getIOType(ti.sid);
            // Пока что IONotifyController поддерживает работу только с 'DI'.
            if( iotype != UniversalIO::DI )
            {
                 ucrit << "(NCRestorer_XML:getThresholdInfo): "
                    << " Bad iotype(" << iotype << ") for " << sid_name << ". iotype must be 'DI'!" << endl;
                return false;
            }

        }
    }

    ti.id             = uit.getIntProp("id");
    ti.lowlimit     = uit.getIntProp("lowlimit");
    ti.hilimit         = uit.getIntProp("hilimit");
    ti.invert         = uit.getIntProp("invert");
    ti.state         = IONotifyController_i::NormalThreshold;
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
void NCRestorer_XML::setNCReadItem( NCReaderSlot sl )
{
    ncrslot = sl;
}
// -----------------------------------------------------------------------------
void NCRestorer_XML::setThresholdsFilter( const std::string& field, const std::string& val )
{
    t_filterField = field;
    t_filterValue = val;
}
// -----------------------------------------------------------------------------
