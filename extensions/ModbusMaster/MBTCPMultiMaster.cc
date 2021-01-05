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
#include <cmath>
#include <limits>
#include <iomanip>
#include <sstream>
#include "unisetstd.h"
#include "Exceptions.h"
#include "extensions/Extensions.h"
#include "MBTCPMultiMaster.h"
#include "modbus/MBLogSugar.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace uniset::extensions;
// -----------------------------------------------------------------------------
MBTCPMultiMaster::MBTCPMultiMaster( uniset::ObjectId objId, uniset::ObjectId shmId,
                                    const std::shared_ptr<SharedMemory>& ic, const std::string& prefix ):
    MBExchange(objId, shmId, ic, prefix),
    force_disconnect(true)
{
    if( objId == DefaultObjectId )
        throw uniset::SystemError("(MBTCPMultiMaster): objId=-1?!! Use --" + prefix + "-name" );

    auto conf = uniset_conf();

    mbconf->prop_prefix = initPropPrefix(mbconf->s_field, "tcp_");
    mbinfo << myname << "(init): prop_prefix=" << mbconf->prop_prefix << endl;

    UniXML::iterator it(cnode);

    checktime = conf->getArgPInt("--" + prefix + "-checktime", it.getProp("checktime"), 5000);
    force_disconnect = conf->getArgInt("--" + prefix + "-persistent-connection", it.getProp("persistent_connection")) ? false : true;

    int ignore_timeout = conf->getArgPInt("--" + prefix + "-ignore-timeout", it.getProp("ignore_timeout"), ptReopen.getInterval());

    // Т.к. при "многоканальном" доступе к slave, смена канала должна происходит сразу после
    // неудачной попытки запросов по одному из каналов, то ПЕРЕОПРЕДЕЛЯЕМ reopen, на channel-timeout..
    int channelTimeout = conf->getArgPInt("--" + prefix + "-default-channel-timeout", it.getProp("channelTimeout"), mbconf->default_timeout);
    ptReopen.setTiming(channelTimeout);

    UniXML::iterator it1(it);

    if( !it1.find("GateList") )
    {
        ostringstream err;
        err << myname << "(init): not found <GateList>";
        mbcrit << err.str() << endl;
        throw uniset::SystemError(err.str());
    }

    if( !it1.goChildren() )
    {
        ostringstream err;
        err << myname << "(init): empty <GateList> ?!";
        mbcrit << err.str() << endl;
        throw uniset::SystemError(err.str());
    }

    for( ; it1.getCurrent(); it1++ )
    {
        if( it1.getIntProp("ignore") )
        {
            mbinfo << myname << "(init): IGNORE " <<  it1.getProp("ip") << ":" << it1.getProp("port") << endl;
            continue;
        }

        auto sinf = make_shared<MBSlaveInfo>();

        sinf->ip = it1.getProp("ip");

        if( sinf->ip.empty() )
        {
            ostringstream err;
            err << myname << "(init): ip='' in <GateList>";
            mbcrit << err.str() << endl;
            throw uniset::SystemError(err.str());
        }

        sinf->port = it1.getIntProp("port");

        if( sinf->port <= 0 )
        {
            ostringstream err;
            err << myname << "(init): ERROR: port=''" << sinf->port << " for ip='" << sinf->ip << "' in <GateList>";
            mbcrit << err.str() << endl;
            throw uniset::SystemError(err.str());
        }

        if( !it1.getProp("respondSensor").empty() )
        {
            sinf->respond_id = conf->getSensorID( it1.getProp("respondSensor") );

            if( sinf->respond_id == DefaultObjectId )
            {
                ostringstream err;
                err << myname << "(init): ERROR: Unknown SensorID for '" << it1.getProp("respondSensor") << "' in <GateList>";
                mbcrit << err.str() << endl;
                throw uniset::SystemError(err.str());
            }
        }

        sinf->priority = it1.getIntProp("priority");
        sinf->mbtcp = std::make_shared<ModbusTCPMaster>();

        sinf->ptIgnoreTimeout.setTiming( it1.getPIntProp("ignore_timeout", ignore_timeout) );
        sinf->recv_timeout = it1.getPIntProp("recv_timeout", mbconf->recv_timeout);
        sinf->aftersend_pause = it1.getPIntProp("aftersend_pause", mbconf->aftersend_pause);
        sinf->sleepPause_usec = it1.getPIntProp("sleepPause_msec", mbconf->sleepPause_msec);
        sinf->respond_invert = it1.getPIntProp("invert", 0);
        sinf->respond_force = it1.getPIntProp("force", 0);

        int fn = conf->getArgPInt("--" + prefix + "-check-func", it.getProp("checkFunc"), ModbusRTU::fnUnknown);

        if( fn != ModbusRTU::fnUnknown &&
                fn != ModbusRTU::fnReadCoilStatus &&
                fn != ModbusRTU::fnReadInputStatus &&
                fn != ModbusRTU::fnReadOutputRegisters &&
                fn != ModbusRTU::fnReadInputRegisters )
        {
            ostringstream err;
            err << myname << "(init):  BAD check function ='" << fn << "'. Must be [1,2,3,4]";
            mbcrit << err.str() << endl;
            throw SystemError(err.str());
        }

        sinf->checkFunc = (ModbusRTU::SlaveFunctionCode)fn;

        sinf->checkAddr = conf->getArgPInt("--" + prefix + "-check-addr", it.getProp("checkAddr"), 0);
        sinf->checkReg = conf->getArgPInt("--" + prefix + "-check-reg", it.getProp("checkReg"), 0);

        int tout = it1.getPIntProp("timeout", channelTimeout);
        sinf->channel_timeout = (tout >= 0 ? tout : channelTimeout);

        // делаем только задержку на отпускание..
        sinf->respondDelay.set(0, sinf->channel_timeout);

        sinf->force_disconnect = it.getPIntProp("persistent_connection", !force_disconnect) ? false : true;

        ostringstream n;
        n << sinf->ip << ":" << sinf->port;
        sinf->myname = n.str();

        auto l = loga->create(sinf->myname);
        sinf->mbtcp->setLog(l);

        mbinfo << myname << "(init): add slave channel " << sinf->myname << endl;
        mblist.emplace_back(sinf);
    }

    if( ic )
        ic->logAgregator()->add(loga);


    if( mblist.empty() )
    {
        ostringstream err;
        err << myname << "(init): empty <GateList>!";
        mbcrit << err.str() << endl;
        throw uniset::SystemError(err.str());
    }

    mblist.sort();
    mbi = mblist.rbegin(); // т.к. mbi это reverse_iterator
    (*mbi)->setUse(true);

    if( shm->isLocalwork() )
        mbconf->loadConfig(conf->getConfXML(), conf->getXMLSensorsSection());
    else
        ic->addReadItem( sigc::mem_fun(this, &MBTCPMultiMaster::readItem) );

    pollThread = unisetstd::make_unique<ThreadCreator<MBTCPMultiMaster>>(this, &MBTCPMultiMaster::poll_thread);
    pollThread->setFinalAction(this, &MBTCPMultiMaster::final_thread);
    checkThread = unisetstd::make_unique<ThreadCreator<MBTCPMultiMaster>>(this, &MBTCPMultiMaster::check_thread);
    checkThread->setFinalAction(this, &MBTCPMultiMaster::final_thread);

    // Т.к. при "многоканальном" доступе к slave, смена канала должна происходит сразу после
    // неудачной попытки запросов по одному из каналов, то ПЕРЕОПРЕДЕЛЯЕМ reopen, на channel-timeout..
    int tout = conf->getArgPInt("--" + prefix + "-default-channel-timeout", it.getProp("channelTimeout"), mbconf->default_timeout);
    ptReopen.setTiming(tout);

    if( mblog->is_info() )
        MBConfig::printMap(mbconf->devices);
}
// -----------------------------------------------------------------------------
MBTCPMultiMaster::~MBTCPMultiMaster()
{
    if( pollThread && !canceled )
    {
        if( pollThread->isRunning() )
            pollThread->join();
    }

    if( checkThread )
    {
        try
        {
            checkThread->stop();

            if( checkThread->isRunning() )
                checkThread->join();
        }
        catch( Poco::NullPointerException& ex )
        {

        }
    }

    mbi = mblist.rend();

}
// -----------------------------------------------------------------------------
std::shared_ptr<ModbusClient> MBTCPMultiMaster::initMB( bool reopen )
{
    if( mb )
        ptInitChannel.reset();

    // просто движемся по кругу (т.к. связь не проверяется)
    // движемся в обратном порядке, т.к. сортировка по возрастанию приоритета
    if( checktime <= 0 )
    {
        ++mbi;

        if( mbi == mblist.rend() )
            mbi = mblist.rbegin();

        auto m = (*mbi);
        m->init(mblog);

        // переопределяем timeout на данный канал
        ptReopen.setTiming( m->channel_timeout );

        m->setUse(true);
        mb = m->mbtcp;
        return m->mbtcp;
    }

    {
        // сперва надо обновить все ignore
        // т.к. фактически флаги выставляются и сбрасываются только здесь
        for( auto&& it : mblist )
            it->ignore = !it->ptIgnoreTimeout.checkTime();

        // если reopen=true - значит почему-то по текущему каналу связи нет (хотя соединение есть)
        // тогда выставляем ему признак игнорирования
        if( mbi != mblist.rend() && reopen )
        {
            auto m = (*mbi);
            m->setUse(false);
            m->ignore = true;
            m->ptIgnoreTimeout.reset();
            mbwarn << myname << "(initMB): set ignore=true for " << m->ip << ":" << m->port << endl;
        }

        // Если по текущему каналу связь есть (и мы его не игнорируем), то возвращаем его
        if( mbi != mblist.rend() && !(*mbi)->ignore && (*mbi)->respond )
        {
            auto m = (*mbi);
            // ещё раз проверим соединение (в неблокирующем режиме)
            m->respond = m->check();

            if( m->respond && (m->mbtcp->isConnection() || m->init(mblog)) )
            {
                mblog4 << myname << "(initMB): SELECT CHANNEL " << m->ip << ":" << m->port << endl;
                mb = m->mbtcp;
                m->setUse(true);
                ptReopen.setTiming( m->channel_timeout );
                return m->mbtcp;
            }

            m->setUse(false);
        }

        if( mbi != mblist.rend() )
            (*mbi)->mbtcp->forceDisconnect();
    }

    // проходим по списку (в обратном порядке, т.к. самый приоритетный в конце)
    for( auto it = mblist.rbegin(); it != mblist.rend(); ++it )
    {
        auto m = (*it);

        if( m->respond && !m->ignore && m->init(mblog) )
        {
            mbi = it;
            mb = m->mbtcp;
            m->setUse(true);
            ptReopen.setTiming( m->channel_timeout );
            mblog4 << myname << "(initMB): SELECT CHANNEL " << m->ip << ":" << m->port << endl;
            return m->mbtcp;
        }
    }

    // если дошли сюда.. значит не нашли ни одного канала..
    // но т.к. мы пропускали те, которые в ignore
    // значит сейчас просто находим первый у кого есть связь и делаем его главным
    for( auto it = mblist.rbegin(); it != mblist.rend(); ++it )
    {
        auto& m = (*it);

        if( m->respond && m->check() && m->init(mblog) )
        {
            mbi = it;
            mb = m->mbtcp;
            m->ignore = false;
            m->setUse(true);
            ptReopen.setTiming( m->channel_timeout );
            mblog4 << myname << "(initMB): SELECT CHANNEL " << m->ip << ":" << m->port << endl;
            return m->mbtcp;
        }
    }

    // значит всё-таки связи реально нет...
    {
        mbi = mblist.rend();
        mb = nullptr;
    }

    return 0;
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::final_thread()
{
    setProcActive(false);
    canceled = true;
}
// -----------------------------------------------------------------------------

bool MBTCPMultiMaster::MBSlaveInfo::check()
{
    std::unique_lock<std::mutex> lock(mutInit, std::try_to_lock);

    // т.к. check вызывается периодически, то нем не страшно сделать return
    // и в следующий раз проверить ещё раз..
    if( !lock.owns_lock() )
        return use; // возвращаем 'use' т.к. если use=1 то считается что связь есть..

    if( !mbtcp )
        return false;

    if( use )
        return true;

    //  cerr << myname << "(check): check connection..." << ip << ":" << port
    //       << " mbfunc=" << checkFunc
    //       << " mbaddr=" << ModbusRTU::addr2str(checkAddr)
    //       << " mbreg=" << (int)checkReg << "(" << ModbusRTU::dat2str(checkReg) << ")"
    //       << endl;

    try
    {
        mbtcp->connect(ip, port, false);

        // результат возврата функции нам не важен
        // т.к. если не будет связи будет выкинуто исключение
        // если пришёл хоть какой-то ответ, значит связь есть
        // (по крайней мере со шлюзом)
        switch(checkFunc)
        {
            case ModbusRTU::fnReadCoilStatus:
            {
                (void)mbtcp->read01(checkAddr, checkReg, 1);
                return true;
            }
            break;

            case ModbusRTU::fnReadInputStatus:
            {
                (void)mbtcp->read02(checkAddr, checkReg, 1);
                return true;
            }
            break;

            case ModbusRTU::fnReadOutputRegisters:
            {
                (void)mbtcp->read03(checkAddr, checkReg, 1);
                return true;
            }
            break;

            case ModbusRTU::fnReadInputRegisters:
            {
                (void)mbtcp->read04(checkAddr, checkReg, 1);
                return true;
            }
            break;

            default: // просто проверка..
                return mbtcp->checkConnection(ip, port, recv_timeout);
        }
    }
    catch(...) {}

    return false;
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::MBSlaveInfo::setUse( bool st )
{
    std::lock_guard<std::mutex> l(mutInit);
    respond_init = !( st && !use );
    use = st;
}
// -----------------------------------------------------------------------------
bool MBTCPMultiMaster::MBSlaveInfo::init( std::shared_ptr<DebugStream>& mblog )
{
    std::lock_guard<std::mutex> l(mutInit);

    mbinfo << myname << "(init): connect..." << endl;

    if( initOK )
        return mbtcp->connect(ip, port);

    mbtcp->connect(ip, port);
    mbtcp->setForceDisconnect(force_disconnect);

    if( recv_timeout > 0 )
        mbtcp->setTimeout(recv_timeout);

    mbtcp->setSleepPause(sleepPause_usec);
    mbtcp->setAfterSendPause(aftersend_pause);
    mbinfo << myname << "(init): connect " << (mbtcp->isConnection() ? "OK" : "FAIL" ) << endl;

    initOK = true;
    return initOK;
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::sysCommand( const uniset::SystemMessage* sm )
{
    MBExchange::sysCommand(sm);

    if( sm->command == SystemMessage::StartUp )
    {
        initCheckConnectionParameters();

        pollThread->start();

        if( checktime > 0 )
            checkThread->start();
    }
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::poll_thread()
{
    // ждём начала работы..(см. MBExchange::activateObject)
    while( !isProcActive() && !canceled )
    {
        uniset::uniset_rwmutex_rlock l(mutex_start);
    }

    // работаем..
    while( isProcActive() )
    {
        try
        {
            if( sidExchangeMode != DefaultObjectId && force )
                exchangeMode = shm->localGetValue(itExchangeMode, sidExchangeMode);
        }
        catch( std::exception& ex )
        {
            mbwarn << myname << "(poll_thread): "  << ex.what() << endl;
        }

        try
        {
            poll();
        }
        catch( std::exception& ex)
        {
            mbwarn << myname << "(poll_thread): "  << ex.what() << endl;
        }

        if( !isProcActive() )
            break;

        msleep(mbconf->polltime);
    }
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::check_thread()
{
    while( isProcActive() )
    {
        for( auto&& it : mblist )
        {
            try
            {
                // сбрасываем флаг ignore..раз время вышло.
                it->ignore = !it->ptIgnoreTimeout.checkTime();

                // Если use=1" связь не проверяем и считаем что связь есть..
                bool r = ( it->use ? true : it->check() );

                mblog4 << myname << "(check): " << it->myname << " " << setw(4) << ( r ? "OK" : "FAIL" )
                       << " [ respondDelay=" << it->respondDelay.check( r )
                       << " timeout=" << it->channel_timeout
                       << " use=" << it->use
                       << " ignore=" << it->ignore
                       << " respond_id=" << it->respond_id
                       << " respond_force=" << it->respond_force
                       << " respond=" << it->respond
                       << " respond_invert=" << it->respond_invert
                       << " activated=" << isProcActive()
                       << " ]"
                       << endl;

                // задержка на выставление "пропажи связи"
                if( it->respond_init )
                    r = it->respondDelay.check( r );

                if( !isProcActive() )
                    break;

                try
                {
                    if( it->respond_id != DefaultObjectId && (it->respond_force || !it->respond_init || r != it->respond) )
                    {
                        bool set = it->respond_invert ? !r : r;
                        shm->localSetValue(it->respond_it, it->respond_id, (set ? 1 : 0), getId());

                        {
                            std::lock_guard<std::mutex> l(it->mutInit);
                            it->respond_init = true;
                        }
                    }
                }
                catch( const uniset::Exception& ex )
                {
                    mbcrit << myname << "(check): (respond) " << it->myname << " : " << ex << std::endl;
                }
                catch( const std::exception& ex )
                {
                    mbcrit << myname << "(check): (respond) " << it->myname << " : " << ex.what() << std::endl;
                }

                it->respond = r;
            }
            catch( const std::exception& ex )
            {
                mbcrit << myname << "(check): (respond) "  << it->myname << " : " << ex.what() << std::endl;
            }

            if( !isProcActive() )
                break;
        }

        if( !isProcActive() )
            break;

        msleep(checktime);
    }
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::initIterators()
{
    MBExchange::initIterators();

    for( auto&& it : mblist )
        shm->initIterator(it->respond_it);
}
// -----------------------------------------------------------------------------
bool MBTCPMultiMaster::deactivateObject()
{
    setProcActive(false);
    canceled = true;

    if( pollThread )
    {
        pollThread->stop();

        if( pollThread->isRunning() )
            pollThread->join();
    }

    if( checkThread )
    {
        checkThread->stop();

        if( checkThread->isRunning() )
            checkThread->join();
    }

    return MBExchange::deactivateObject();
}
// -----------------------------------------------------------------------------
void MBTCPMultiMaster::initCheckConnectionParameters()
{
    auto conf = uniset_conf();

    bool initFromRegMap = ( findArgParam("--" + mbconf->prefix + "-check-init-from-regmap", conf->getArgc(), conf->getArgv()) != -1 );

    if( !initFromRegMap )
        return;

    mbinfo << myname << "(init): init check connection parameters from regmap.." << endl;



    // берём первый попавшийся read-регистр из списка
    // от первого попавшегося устройства..

    ModbusRTU::SlaveFunctionCode checkFunc = ModbusRTU::fnUnknown;
    ModbusRTU::ModbusAddr checkAddr = { 0x00 };
    ModbusRTU::ModbusData checkReg = { 0 };

    if( mbconf->devices.empty() )
    {
        mbwarn << myname << "(init): devices list empty?!" << endl;
        return;
    }

    // идём по устройствам
    for( const auto& d : mbconf->devices )
    {
        checkAddr = d.second->mbaddr;

        if( d.second->pollmap.empty() )
            continue;

        // идём по списку опрашиваемых регистров
        for( auto p = d.second->pollmap.begin(); p != d.second->pollmap.end(); ++p )
        {
            for( auto r = p->second->begin(); r != p->second->end(); ++r )
            {
                if( ModbusRTU::isReadFunction(r->second->mbfunc) )
                {
                    checkFunc = r->second->mbfunc;
                    checkReg = r->second->mbreg;
                    break;
                }
            }

            if( checkFunc != ModbusRTU::fnUnknown )
                break;
        }

        if( checkFunc != ModbusRTU::fnUnknown )
            break;
    }

    if( checkFunc == ModbusRTU::fnUnknown )
    {
        ostringstream err;

        err << myname << "(init): init check connection parameters: ERROR: "
            << " NOT FOUND read-registers for check connection!"
            << endl;

        mbcrit << err.str() << endl;
        throw SystemError(err.str());
    }

    mbinfo << myname << "(init): init check connection parameters: "
           << " mbfunc=" << checkFunc
           << " mbaddr=" << ModbusRTU::addr2str(checkAddr)
           << " mbreg=" << (int)checkReg << "(" << ModbusRTU::dat2str(checkReg) << ")"
           << endl;

    // инициализируем..
    for( auto&& m : mblist )
    {
        m->checkFunc = checkFunc;
        m->checkAddr = checkAddr;
        m->checkReg = checkReg;
    }
}

// -----------------------------------------------------------------------------
void MBTCPMultiMaster::help_print( int argc, const char* const* argv )
{
    cout << "Default: prefix='mbtcp'" << endl;
    MBExchange::help_print(argc, argv);
    cout << endl;
    cout << " Настройки протокола TCP(MultiMaster): " << endl;
    cout << "--prefix-persistent-connection 0,1 - Не закрывать соединение на каждом цикле опроса" << endl;
    cout << "--prefix-checktime msec            - период проверки связи по каналам (<GateList>)" << endl;
    cout << "--prefix-ignore-timeout msec       - Timeout на повторную попытку использования канала после 'reopen-timeout'. По умолчанию: reopen-timeout * 3" << endl;

    cout << "--prefix-check-func [1,2,3,4]      - Номер функции для проверки соединения" << endl;
    cout << "--prefix-check-addr [1..255 ]      - Адрес устройства для проверки соединения" << endl;
    cout << "--prefix-check-reg [1..65535]      - Регистр для проверки соединения" << endl;

    cout << endl;
    cout << " ВНИМАНИЕ! '--prefix-reopen-timeout' для MBTCPMultiMaster НЕ ДЕЙСТВУЕТ! " << endl;
    cout << " Смена канала происходит по --prefix-timeout. " << endl;
    cout << " При этом следует учитывать блокировку отключаемого канала на время: --prefix-ignore-timeout" << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<MBTCPMultiMaster> MBTCPMultiMaster::init_mbmaster( int argc, const char* const* argv,
        uniset::ObjectId icID, const std::shared_ptr<SharedMemory>& ic,
        const std::string& prefix )
{
    auto conf = uniset_conf();

    string name = conf->getArgParam("--" + prefix + "-name", "MBTCPMultiMaster1");

    if( name.empty() )
    {
        dcrit << "(MBTCPMultiMaster): Не задан name'" << endl;
        return 0;
    }

    ObjectId ID = conf->getObjectID(name);

    if( ID == uniset::DefaultObjectId )
    {
        dcrit << "(MBTCPMultiMaster): идентификатор '" << name
              << "' не найден в конф. файле!"
              << " в секции " << conf->getObjectsSection() << endl;
        return 0;
    }

    dinfo << "(MBTCPMultiMaster): name = " << name << "(" << ID << ")" << endl;
    return make_shared<MBTCPMultiMaster>(ID, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
const std::string MBTCPMultiMaster::MBSlaveInfo::getShortInfo() const
{
    ostringstream s;
    s << myname << " respond=" << respond
      << " (respond_id=" << respond_id
      << " respond_invert=" << respond_invert
      << " recv_timeout=" << recv_timeout
      << " resp_force=" << respond_force
      << " use=" << use
      << " ignore=" << ( ptIgnoreTimeout.checkTime() ? "0" : "1")
      << " priority=" << priority
      << " persistent-connection=" << !force_disconnect
      << ")";

    return s.str();
}
// -----------------------------------------------------------------------------
uniset::SimpleInfo* MBTCPMultiMaster::getInfo( const char* userparam )
{
    uniset::SimpleInfo_var i = MBExchange::getInfo(userparam);

    ostringstream inf;

    inf << i->info << endl;
    inf << "Gates: " << (checktime <= 0 ? "/ check connections DISABLED /" : "") << endl;

    for( const auto& m : mblist )
        inf << "   " << m->getShortInfo() << endl;

    inf << endl;

    i->info = inf.str().c_str();
    return i._retn();
}
// ----------------------------------------------------------------------------

