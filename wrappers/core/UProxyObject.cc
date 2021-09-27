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
#include <cmath>
#include <sstream>
#include <mutex>
#include <string>
#include <unordered_map>
#include "Configuration.h"
#include "extensions/UObject_SK.h"
#include "UniSetActivator.h"
#include "UProxyObject.h"
// --------------------------------------------------------------------------
using namespace uniset;
// --------------------------------------------------------------------------
/*! PIMPL  реализация UProxyObject */
class UProxyObject_impl:
    public UObject_SK
{
    public:

        UProxyObject_impl( uniset::ObjectId id );
        virtual ~UProxyObject_impl();

        void impl_addToAsk( uniset::ObjectId id ) ;
        void impl_askSensor( uniset::ObjectId id ) ;

        long impl_getValue( long id ) ;
        void impl_setValue( long id, long val ) ;
        float impl_getFloatValue( long id ) ;

        bool impl_askIsOK();
        bool impl_reaskSensors();
        bool impl_updateValues();
        bool impl_smIsOK();

    protected:
        virtual void askSensors( UniversalIO::UIOCommand cmd ) override;
        virtual void sensorInfo( const uniset::SensorMessage* sm ) override;

    private:

        struct SInfo
        {
            SInfo()
            {
                si.id = uniset::DefaultObjectId;
                si.node = uniset::DefaultObjectId;
            }

            IOController_i::SensorInfo si;
            long value = { 0 };
            float fvalue = { 0.0 };
            long precision = { 0 };
        };

        std::mutex mutexSMap;
        std::unordered_map<uniset::ObjectId, SInfo> smap;
        bool askOK = { false };
};
// --------------------------------------------------------------------------
UProxyObject::UProxyObject() 
{
    throw UException("(UProxyObject): Unknown 'name'' or 'ID'");
}
// --------------------------------------------------------------------------
UProxyObject::UProxyObject( const std::string& name ) 
{
    auto conf = uniset_conf();

    if ( !conf )
    {
        std::ostringstream err;
        err << "(UProxyObject:init): Create '" << name << "' failed. Unknown configuration";
        std::cerr << err.str() << std::endl;
        throw UException(err.str());
    }

    long id = conf->getObjectID(name);
    init(id);
}
// --------------------------------------------------------------------------
UProxyObject::UProxyObject( long id )
{
    init(id);
}
// --------------------------------------------------------------------------
void UProxyObject::init( long id )
{
    try
    {
        uobj = std::make_shared<UProxyObject_impl>(id);
        auto act = UniSetActivator::Instance();
        act->add(uobj);
    }
    catch( std::exception& ex )
    {
        std::ostringstream err;
        err << "(UProxyObject): id='" << id << "' error: " << std::string(ex.what());
        std::cerr << err.str() << std::endl;
        throw UException(err.str());
    }
}
// --------------------------------------------------------------------------
UProxyObject::~UProxyObject()
{
}
// --------------------------------------------------------------------------
long UProxyObject::getValue( long id ) 
{
    return uobj->impl_getValue(id);
}
// --------------------------------------------------------------------------
void UProxyObject::setValue( long id, long val ) 
{
    uobj->impl_setValue(id, val);
}
// --------------------------------------------------------------------------
bool UProxyObject::askIsOK()
{
    return uobj->impl_askIsOK();
}
// --------------------------------------------------------------------------
bool UProxyObject::reaskSensors()
{
    return uobj->impl_reaskSensors();
}
// --------------------------------------------------------------------------
bool UProxyObject::updateValues()
{
    return uobj->impl_updateValues();
}
// --------------------------------------------------------------------------
bool UProxyObject::smIsOK()
{
    return uobj->impl_smIsOK();
}
// --------------------------------------------------------------------------
float UProxyObject::getFloatValue( long id ) 
{
    return uobj->impl_getFloatValue(id);
}
// --------------------------------------------------------------------------
void UProxyObject::addToAsk( long id ) 
{
    try
    {
        uobj->impl_addToAsk(id);
    }
    catch( std::exception& ex )
    {
        std::ostringstream err;
        err << uobj->getName() << "(addToAsk): " << id << " error: " << std::string(ex.what());
        throw UException(err.str());
    }
}
// --------------------------------------------------------------------------
void UProxyObject::askSensor( long id ) 
{
    try
    {
        uobj->impl_askSensor(id);
    }
    catch( std::exception& ex )
    {
        std::ostringstream err;
        err << uobj->getName() << "(askSensor): " << id << " error: " << std::string(ex.what());
        throw UException(err.str());
    }
}
// --------------------------------------------------------------------------
UProxyObject_impl::UProxyObject_impl( ObjectId id ):
    UObject_SK(id, nullptr)
{

}
// --------------------------------------------------------------------------
UProxyObject_impl::~UProxyObject_impl()
{

}
// --------------------------------------------------------------------------
void UProxyObject_impl::impl_addToAsk( ObjectId id ) 
{
    auto conf = uniset_conf();

    UProxyObject_impl::SInfo i;
    i.si.id = id;
    i.si.node = conf->getLocalNode();

    auto inf = conf->oind->getObjectInfo(id);

    if( inf && inf->xmlnode )
    {
        UniXML::iterator it( inf->xmlnode );
        i.precision = it.getIntProp("precision");
    }

    std::unique_lock<std::mutex> lk(mutexSMap);
    smap[id] = i;
}
// --------------------------------------------------------------------------
long UProxyObject_impl::impl_getValue( long id ) 
{
    std::unique_lock<std::mutex> lk(mutexSMap);
    auto i = smap.find(id);

    if( i == smap.end() )
    {
        std::ostringstream err;
        err << myname << "(getValue): " << id << "  not found in proxy sensors list..";
        throw UException(err.str());
    }

    return i->second.value;
}
// --------------------------------------------------------------------------
float UProxyObject_impl::impl_getFloatValue( long id ) 
{
    std::unique_lock<std::mutex> lk(mutexSMap);
    auto i = smap.find(id);

    if( i == smap.end() )
    {
        std::ostringstream err;
        err << myname << "(getFloatValue): " << id << "  not found in proxy sensors list..";
        throw UException(err.str());
    }

    return i->second.fvalue;
}
// --------------------------------------------------------------------------
bool UProxyObject_impl::impl_askIsOK()
{
    std::unique_lock<std::mutex> lk(mutexSMap);
    return askOK;
}
// --------------------------------------------------------------------------
void UProxyObject_impl::impl_setValue( long id, long val ) 
{
    try
    {
        UObject_SK::setValue(id, val);
    }
    catch( std::exception& ex )
    {
        std::ostringstream err;
        err << myname << "(setValue): " << id << " error: " << std::string(ex.what());
        throw UException(err.str());
    }
}
// --------------------------------------------------------------------------
bool UProxyObject_impl::impl_reaskSensors()
{
    askSensors(UniversalIO::UIONotify);
    return impl_askIsOK();
}
// --------------------------------------------------------------------------
bool UProxyObject_impl::impl_updateValues()
{
    std::unique_lock<std::mutex> lk(mutexSMap);
    bool ret = true;

    for( auto&& i : smap )
    {
        try
        {
            i.second.value = ui->getValue(i.second.si.id, i.second.si.node);
            i.second.fvalue = (float)i.second.value / pow(10.0, i.second.precision);
        }
        catch( std::exception& ex )
        {
            mycrit << myname << "(updateValues): " << i.second.si.id << " error: " << ex.what() << std::endl;
            ret = false;
        }
    }

    return ret;
}
// --------------------------------------------------------------------------
bool UProxyObject_impl::impl_smIsOK()
{
    std::unique_lock<std::mutex> lk(mutexSMap);

    // проверяем по первому датчику
    auto s = smap.begin();
    return ui->isExist(s->second.si.id, s->second.si.node);
}
// --------------------------------------------------------------------------
void UProxyObject_impl::impl_askSensor( uniset::ObjectId id ) 
{
    ui->askRemoteSensor(id, UniversalIO::UIONotify, uniset_conf()->getLocalNode(), getId());
    impl_addToAsk(id);
}
// --------------------------------------------------------------------------
void UProxyObject_impl::askSensors( UniversalIO::UIOCommand cmd )
{
    std::unique_lock<std::mutex> lk(mutexSMap);
    askOK = true;

    for( const auto& i : smap )
    {
        try
        {
            ui->askRemoteSensor(i.second.si.id, cmd, i.second.si.node, getId());
        }
        catch( std::exception& ex )
        {
            mywarn << myname << "(askSensors): " << i.second.si.id << " error: " << ex.what() << std::endl;
            askOK = false;
        }
    }
}
// --------------------------------------------------------------------------
void UProxyObject_impl::sensorInfo( const SensorMessage* sm )
{
    std::unique_lock<std::mutex> lk(mutexSMap);
    auto i = smap.find(sm->id);

    if( i != smap.end() )
    {
        i->second.value = sm->value;
        i->second.fvalue = (float)sm->value / pow(10.0, sm->ci.precision);
    }
}
// --------------------------------------------------------------------------
