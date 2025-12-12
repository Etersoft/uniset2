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
#include "UConnector.h"
#include "ORepHelpers.h"
#include "MessageType.h"
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
UConnector::UConnector( UTypes::Params* p, const std::string& xfile ):
    xmlfile(xfile)
{
    try
    {
        conf = uniset::uniset_init(p->argc, p->argv, xmlfile);
        ui = make_shared<uniset::UInterface>(conf);
    }
    catch( std::exception& ex )
    {
        throw UException(ex.what());
    }
    catch( ... )
    {
        throw UException("(UConnector): Unknown exception");
    }
}
//---------------------------------------------------------------------------
UConnector::UConnector(int argc, char** argv, const string& xfile ):
    xmlfile(xfile)
{
    try
    {
        conf = uniset::uniset_init(argc, argv, xmlfile);
        ui = make_shared<uniset::UInterface>(conf);
    }
    catch( std::exception& ex )
    {
        throw UException(ex.what());
    }
    catch( ... )
    {
        throw UException("(UConnector): Unknown exception");
    }
}
// --------------------------------------------------------------------------
UConnector::~UConnector()
{
}
// --------------------------------------------------------------------------
string UConnector::getConfFileName()
{
    //    return xmlfile;
    if( conf )
        return conf->getConfFileName();

    return "";

}
// --------------------------------------------------------------------------
long UConnector::getValue( long id, long node )
{
    if( !conf || !ui )
        throw USysError();

    if( node == UTypes::DefaultID )
        node = conf->getLocalNode();

    try
    {
        return ui->getValue(id, node);
    }
    catch( uniset::Exception& ex )
    {
        throw UException(ex.what());
    }
    catch( std::exception& ex )
    {
        throw UException(ex.what());
    }
}
//---------------------------------------------------------------------------
void UConnector::setValue( long id, long val, long node, long supplier )
{
    if( !conf || !ui )
        throw USysError();


    if( node == UTypes::DefaultID )
        node = conf->getLocalNode();

    try
    {
        ui->setValue(id, val, node, supplier);
    }
    catch( uniset::Exception& ex )
    {
        throw UException(ex.what());
    }
    catch( std::exception& ex )
    {
        throw UException(ex.what());
    }
}
//---------------------------------------------------------------------------
static UTypes::ShortIOInfo toUTypes( IOController_i::ShortIOInfo i )
{
    UTypes::ShortIOInfo ret;
    ret.value = i.value;
    ret.tv_sec = i.tv_sec;
    ret.tv_nsec = i.tv_nsec;
    ret.supplier = i.supplier;
    ret.supplier_node = UTypes::DefaultID;

    return ret;
}
//---------------------------------------------------------------------------
UTypes::ShortIOInfo UConnector::getTimeChange( long id, long node )
{
    if( !conf || !ui )
        throw USysError();

    if( node == UTypes::DefaultID )
        node = conf->getLocalNode();

    try
    {
        IOController_i::ShortIOInfo i = ui->getTimeChange(id, node);
        return toUTypes(i);
    }
    catch( const std::exception& ex )
    {
        throw UException("(getChangedTime): catch " + std::string(ex.what()) );
    }
}
//---------------------------------------------------------------------------
long UConnector::getSensorID( const string& name )
{
    if( conf )
        return conf->getSensorID(name);

    return UTypes::DefaultID;
}
//---------------------------------------------------------------------------
long UConnector::getNodeID(const string& name )
{
    if( conf )
        return conf->getNodeID(name);

    return UTypes::DefaultID;
}
//---------------------------------------------------------------------------
string UConnector::getName( long id )
{
    if( conf )
        return conf->oind->getMapName(id);

    return "";
}
//---------------------------------------------------------------------------
string UConnector::getShortName( long id )
{
    if( conf )
        return conf->oind->getShortName(id);

    return "";
}
//---------------------------------------------------------------------------
string UConnector::getTextName( long id )
{
    if( conf )
        return conf->oind->getTextName(id);

    return "";
}
//---------------------------------------------------------------------------
string UConnector::getObjectInfo( long id, const std::string& params, long node )
{
    if( !conf || !ui )
        throw USysError();

    if( id == UTypes::DefaultID )
        throw UException("(getObjectInfo): Unknown ID..");

    if( node == UTypes::DefaultID )
        node = conf->getLocalNode();

    try
    {
        return ui->getObjectInfo(id, params, node);
    }
    catch( std::exception& ex )
    {
        throw UException("(getObjectInfo): error: " + std::string(ex.what()) );
    }
}
//---------------------------------------------------------------------------
string UConnector::apiRequest( long id, const string& query, long node )
{
    if( !conf || !ui )
        throw USysError();

    if( id == UTypes::DefaultID )
        throw UException("(apiRequest): Unknown ID..");

    if( node == UTypes::DefaultID )
        node = conf->getLocalNode();

    try
    {
        return ui->apiRequest(id, query, node);
    }
    catch( std::exception& ex )
    {
        throw UException("(apiRequest): error: " + std::string(ex.what()) );
    }
}
//---------------------------------------------------------------------------
void UConnector::activate_objects()
{
    try
    {
        auto act = uniset::UniSetActivator::Instance();
        uniset::SystemMessage sm(uniset::SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(true);
    }
    catch( const std::exception& ex )
    {
        throw UException("(activate_objects): catch " + std::string(ex.what()) );
    }
}
//---------------------------------------------------------------------------
long UConnector::getObjectID( const string& name )
{
    if( conf )
    {
        long id = conf->getObjectID(name);

        if( id == UTypes::DefaultID )
            id = conf->getControllerID(name);

        if( id == UTypes::DefaultID )
            id = conf->getServiceID(name);

        return id;
    }

    return UTypes::DefaultID;
}
//---------------------------------------------------------------------------
