/*
 * Copyright (c) 2020 Pavel Vainerman.
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
#include <sstream>
#include <Exceptions.h>
#include <UniSetTypes.h>
#include <extensions/Extensions.h>
#include <ORepHelpers.h>
#include "MBConfig.h"
#include "modbus/MBLogSugar.h"
// -------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    using namespace std;
    using namespace uniset::extensions;
    // -----------------------------------------------------------------------------
    MBConfig::MBConfig(const std::shared_ptr<uniset::Configuration>& _conf
                       , xmlNode* _cnode
                       , std::shared_ptr<SMInterface> _shm ):
        cnode(_cnode),
        conf(_conf),
        shm(_shm)
    {
    }
    // -----------------------------------------------------------------------------
    MBConfig::~MBConfig()
    {
    }
    // -----------------------------------------------------------------------------
    void MBConfig::cloneParams( const std::shared_ptr<MBConfig>& mbconf )
    {
        s_field = mbconf->s_field;
        s_fvalue = mbconf->s_fvalue;
        defaultMBtype = mbconf->defaultMBtype;
        defaultMBaddr = mbconf->defaultMBaddr;
        defaultMBinitOK = mbconf->defaultMBinitOK;
        mblog = mbconf->mblog;
        myname = mbconf->myname;
        noQueryOptimization = mbconf->noQueryOptimization;
        maxQueryCount = mbconf->maxQueryCount;

        recv_timeout = mbconf->recv_timeout;
        default_timeout = mbconf->default_timeout;
        aftersend_pause = mbconf->aftersend_pause;
        polltime = mbconf->polltime;
        sleepPause_msec = mbconf->sleepPause_msec;
        prefix = mbconf->prefix;
        prop_prefix = mbconf->prop_prefix;
        mbregFromID = mbconf->mbregFromID;
    }
    // -----------------------------------------------------------------------------
    void MBConfig::loadConfig( const std::shared_ptr<uniset::UniXML>& xml, UniXML::iterator sensorsSection )
    {
        readConfiguration(xml, sensorsSection);

        if( !noQueryOptimization )
            rtuQueryOptimization(devices, maxQueryCount);

        initDeviceList(xml);
    }
    // ------------------------------------------------------------------------------------------
    void MBConfig::readConfiguration( const std::shared_ptr<uniset::UniXML>& xml, UniXML::iterator sensorsSection )
    {
        UniXML::iterator it = sensorsSection;

        if( !it.getCurrent() )
        {
            it = xml->findNode(xml->getFirstNode(), "sensors");

            if(!it)
            {
                ostringstream err;
                err << myname << "(readConfiguration): не нашли корневого раздела <sensors>";
                throw SystemError(err.str());
            }
        }

        if( !it.goChildren() )
        {
            mbcrit << myname << "(readConfiguration): раздел <sensors> не содержит секций ?!!\n";
            return;
        }

        for( ; it.getCurrent(); it.goNext() )
        {
            if( uniset::check_filter(it, s_field, s_fvalue) )
                initItem(it);
        }

        //    readconf_ok = true;
    }
    // ------------------------------------------------------------------------------------------
    MBConfig::DeviceType MBConfig::getDeviceType( const std::string& dtype ) noexcept
    {
        if( dtype.empty() )
            return dtUnknown;

        if( dtype == "mtr" || dtype == "MTR" )
            return dtMTR;

        if( dtype == "rtu" || dtype == "RTU" )
            return dtRTU;

        if( dtype == "rtu188" || dtype == "RTU188" )
            return dtRTU188;

        return dtUnknown;
    }
    // -----------------------------------------------------------------------------
    void MBConfig::printMap( MBConfig::RTUDeviceMap& m )
    {
        cout << "devices: " << endl;

        for( auto it = m.begin(); it != m.end(); ++it )
            cout << "  " <<  *(it->second) << endl;
    }
    // -----------------------------------------------------------------------------
    std::ostream& operator<<( std::ostream& os, MBConfig::RTUDeviceMap& m )
    {
        os << "devices: " << endl;

        for( auto it = m.begin(); it != m.end(); ++it )
            os << "  " <<  *(it->second) << endl;

        return os;
    }
    // -----------------------------------------------------------------------------
    std::ostream& operator<<( std::ostream& os, MBConfig::RTUDevice& d )
    {
        os  << "addr=" << ModbusRTU::addr2str(d.mbaddr)
            << " type=" << d.dtype
            << " respond_id=" << d.resp_id
            << " respond_timeout=" << d.resp_Delay.getOffDelay()
            << " respond_state=" << d.resp_state
            << " respond_invert=" << d.resp_invert
            << " safeMode=" << (MBConfig::SafeMode)d.safeMode
            << endl;


        os << "  regs: " << endl;

        for( const auto& m : d.pollmap )
        {
            for( const auto& it : * (m.second) )
                os << "     " << it.second << endl;
        }

        return os;
    }
    // -----------------------------------------------------------------------------
    std::ostream& operator<<( std::ostream& os, const MBConfig::RegInfo* r )
    {
        return os << (*r);
    }
    // -----------------------------------------------------------------------------
    std::ostream& operator<<( std::ostream& os, const MBConfig::RegInfo& r )
    {
        os << " id=" << r.regID
           << " mbreg=" << ModbusRTU::dat2str(r.mbreg)
           << " mbfunc=" << r.mbfunc
           << " q_num=" << r.q_num
           << " q_count=" << r.q_count
           << " value=" << ModbusRTU::dat2str(r.mbval) << "(" << (int)r.mbval << ")"
           << " mtrType=" << MTR::type2str(r.mtrType)
           << endl;

        for( const auto& s : r.slst )
            os << "         " << s << endl;

        return os;
    }
    // -----------------------------------------------------------------------------
    void MBConfig::rtuQueryOptimization( RTUDeviceMap& dm, size_t maxQueryCount )
    {
        //      mbinfo << myname << "(rtuQueryOptimization): optimization..." << endl;

        for( const auto& d : dm )
            rtuQueryOptimizationForDevice(d.second, maxQueryCount);
    }
    // -----------------------------------------------------------------------------
    void MBConfig::rtuQueryOptimizationForDevice( const std::shared_ptr<RTUDevice>& d, size_t maxQueryCount )
    {
        //      mbinfo << myname << "(rtuQueryOptimizationForDevice): dev addr="
        //             << ModbusRTU::addr2str(d->mbaddr) << " optimization..." << endl;

        for( const auto& m : d->pollmap )
            rtuQueryOptimizationForRegMap(m.second, maxQueryCount);
    }
    // -----------------------------------------------------------------------------
    void MBConfig::rtuQueryOptimizationForRegMap( const std::shared_ptr<RegMap>& regmap, size_t maxQueryCount )
    {
        if( regmap->size() <= 1 )
            return;

        // Вообще в map они уже лежат в нужном порядке, т.е. функция genRegID() гарантирует
        // что регистры идущие подряд с одниковой функцией чтения/записи получат подряд идущие RegID.
        // так что оптимтизация это просто нахождение мест где RegID идут не подряд...
        for( auto it = regmap->begin(); it != regmap->end(); ++it )
        {
            auto& beg = it->second;
            ModbusRTU::RegID regID = beg->regID;
            beg->q_count = 1;
            beg->q_num = 0;
            ++it;

            // склеиваем регистры идущие подряд
            for( ; it != regmap->end(); ++it )
            {
                if( (it->second->regID - regID) > 1 )
                {
                    // этот регистр должен войти уже в следующий запрос,
                    // надо вернуть на шаг обратно..
                    --it;
                    break;
                }

                beg->q_count++;
                regID = it->second->regID;
                it->second->q_num = beg->q_count - 1;
                it->second->q_count = 0;

                if( beg->q_count >= maxQueryCount )
                    break;
            }

            // Корректировка типа функции, в случае необходимости...
            if( beg->q_count > 1 && beg->mbfunc == ModbusRTU::fnWriteOutputSingleRegister )
            {
                //              mbwarn << myname << "(rtuQueryOptimization): "
                //                     << " optimization change func=" << ModbusRTU::fnWriteOutputSingleRegister
                //                     << " <--> func=" << ModbusRTU::fnWriteOutputRegisters
                //                     << " for mbaddr=" << ModbusRTU::addr2str(beg->dev->mbaddr)
                //                     << " mbreg=" << ModbusRTU::dat2str(beg->mbreg);

                beg->mbfunc = ModbusRTU::fnWriteOutputRegisters;
            }
            else if( beg->q_count > 1 && beg->mbfunc == ModbusRTU::fnForceSingleCoil )
            {
                //              mbwarn << myname << "(rtuQueryOptimization): "
                //                     << " optimization change func=" << ModbusRTU::fnForceSingleCoil
                //                     << " <--> func=" << ModbusRTU::fnForceMultipleCoils
                //                     << " for mbaddr=" << ModbusRTU::addr2str(beg->dev->mbaddr)
                //                     << " mbreg=" << ModbusRTU::dat2str(beg->mbreg);

                beg->mbfunc = ModbusRTU::fnForceMultipleCoils;
            }

            // надо до внешнего цикла, где будет ++it
            // проверить условие.. (т.к. мы во внутреннем цикле итерировались
            if( it == regmap->end() )
                break;
        }
    }
    // -----------------------------------------------------------------------------
    //std::ostream& operator<<( std::ostream& os, MBConfig::PList& lst )
    std::ostream& MBConfig::print_plist( std::ostream& os, const MBConfig::PList& lst )
    {
        os << "[ ";

        for( const auto& p : lst )
            os << "(" << p.si.id << ")" << conf->oind->getBaseName(conf->oind->getMapName(p.si.id)) << " ";

        os << "]";

        return os;
    }
    // -----------------------------------------------------------------------------
    std::shared_ptr<MBConfig::RTUDevice> MBConfig::addDev( RTUDeviceMap& mp, ModbusRTU::ModbusAddr a, UniXML::iterator& xmlit )
    {
        auto it = mp.find(a);

        if( it != mp.end() )
        {
            string s_dtype(xmlit.getProp(prop_prefix + "mbtype"));

            if( s_dtype.empty() )
                s_dtype = defaultMBtype;

            DeviceType dtype = getDeviceType(s_dtype);

            if( it->second->dtype != dtype )
            {
                mbcrit << myname << "(addDev): OTHER mbtype=" << dtype << " for " << xmlit.getProp("name")
                       << " prop='" << prop_prefix + "mbtype" << "'"
                       << ". Already used devtype=" <<  it->second->dtype
                       << " for mbaddr=" << ModbusRTU::addr2str(it->second->mbaddr)
                       << endl;
                return 0;
            }

            mbinfo << myname << "(addDev): device for addr=" << ModbusRTU::addr2str(a)
                   << " already added. Ignore device params for " << xmlit.getProp("name") << " ..." << endl;
            return it->second;
        }

        auto d = make_shared<MBConfig::RTUDevice>();
        d->mbaddr = a;

        if( !initRTUDevice(d, xmlit) )
        {
            d.reset();
            return 0;
        }

        mp.insert( std::make_pair(a, d) );
        return d;
    }
    // ------------------------------------------------------------------------------------------
    std::shared_ptr<MBConfig::RegInfo> MBConfig::addReg( std::shared_ptr<RegMap>& mp, ModbusRTU::RegID regID, ModbusRTU::ModbusData r,
            UniXML::iterator& xmlit, std::shared_ptr<MBConfig::RTUDevice> dev )
    {
        auto it = mp->find(regID);

        if( it != mp->end() )
        {
            if( !it->second->dev )
            {
                mbcrit << myname << "(addReg): for " << xmlit.getProp("name")
                       << " dev=0!!!! " << endl;
                return 0;
            }

            if( it->second->dev->dtype != dev->dtype )
            {
                mbcrit << myname << "(addReg): OTHER mbtype=" << dev->dtype << " for " << xmlit.getProp("name")
                       << ". Already used devtype=" <<  it->second->dev->dtype << " for " << it->second->dev << endl;
                return 0;
            }

            mbinfo << myname << "(addReg): reg=" << ModbusRTU::dat2str(r)
                   << "(regID=" << regID << ")"
                   << " already added for " << (*it->second)
                   << " Ignore register params for " << xmlit.getProp("name") << " ..." << endl;

            it->second->rit = it;
            return it->second;
        }

        auto ri = make_shared<MBConfig::RegInfo>();

        if( !initRegInfo(ri, xmlit, dev) )
            return 0;

        ri->mbreg = r;
        ri->regID = regID;

        mp->insert( std::make_pair(regID, ri) );
        ri->rit = mp->find(regID);

        mbinfo << myname << "(addReg): reg=" << ModbusRTU::dat2str(r) << "(regID=" << regID << ")" << endl;

        return ri;
    }
    // ------------------------------------------------------------------------------------------
    MBConfig::RSProperty* MBConfig::addProp( PList& plist, RSProperty&& p )
    {
        for( auto&& it : plist )
        {
            if( it.si.id == p.si.id && it.si.node == p.si.node )
                return &it;
        }

        plist.emplace_back( std::move(p) );
        auto it = plist.end();
        --it;
        return &(*it);
    }
    // ------------------------------------------------------------------------------------------
    bool MBConfig::initRSProperty( RSProperty& p, UniXML::iterator& it )
    {
        if( !IOBase::initItem(&p, it, shm, prop_prefix, false, mblog, myname) )
            return false;

        // проверяем не пороговый ли это датчик (т.е. не связанный с обменом)
        // тогда заносим его в отдельный список
        if( p.t_ai != DefaultObjectId )
        {
            thrlist.emplace_back( std::move(p) );
            return true;
        }

        const string sbit(IOBase::initProp(it, "nbit", prop_prefix, false));

        if( !sbit.empty() )
        {
            p.nbit = uniset::uni_atoi(sbit.c_str());

            if( p.nbit < 0 || p.nbit >= ModbusRTU::BitsPerData )
            {
                mbcrit << myname << "(initRSProperty): BAD nbit=" << (int)p.nbit
                       << ". (0 >= nbit < " << ModbusRTU::BitsPerData << ")." << endl;
                return false;
            }
        }

        if( p.nbit > 0 &&
                ( p.stype == UniversalIO::AI ||
                  p.stype == UniversalIO::AO ) )
        {
            mbwarn << "(initRSProperty): (ignore) uncorrect param`s nbit!=0(" << p.nbit << ")"
                   << " for iotype=" << p.stype << " for " << it.getProp("name") << endl;
        }

        const string sbyte(IOBase::initProp(it, "nbyte", prop_prefix, false) );

        if( !sbyte.empty() )
        {
            p.nbyte = uniset::uni_atoi(sbyte.c_str());

            if( p.nbyte > VTypes::Byte::bsize )
            {
                mbwarn << myname << "(initRSProperty): BAD nbyte=" << p.nbyte
                       << ". (0 >= nbyte < " << VTypes::Byte::bsize << ")." << endl;
                return false;
            }
        }

        const string vt( IOBase::initProp(it, "vtype", prop_prefix, false) );

        if( vt.empty() )
        {
            p.rnum = VTypes::wsize(VTypes::vtUnknown);
            p.vType = VTypes::vtUnknown;
        }
        else
        {
            VTypes::VType v(VTypes::str2type(vt));

            if( v == VTypes::vtUnknown )
            {
                mbcrit << myname << "(initRSProperty): Unknown tcp_vtype='" << vt << "' for "
                       << it.getProp("name")
                       << endl;

                return false;
            }

            p.vType = v;
            p.rnum = VTypes::wsize(v);
        }

        return true;
    }
    // ------------------------------------------------------------------------------------------
    bool MBConfig::initRegInfo( std::shared_ptr<RegInfo>& r, UniXML::iterator& it,  std::shared_ptr<MBConfig::RTUDevice>& dev )
    {
        r->dev = dev;
        r->mbval = IOBase::initIntProp(it, "default", prop_prefix, false);

        if( dev->dtype == MBConfig::dtRTU )
        {
            //        mblog.info() << myname << "(initRegInfo): init RTU.."
        }
        else if( dev->dtype == MBConfig::dtMTR )
        {
            // only for MTR
            if( !initMTRitem(it, r) )
                return false;
        }
        else if( dev->dtype == MBConfig::dtRTU188 )
        {
            // only for RTU188
            if( !initRTU188item(it, r) )
                return false;

            UniversalIO::IOType t = uniset::getIOType(IOBase::initProp(it, "iotype", prop_prefix, false));
            r->mbreg = RTUStorage::getRegister(r->rtuJack, r->rtuChan, t);
            r->mbfunc = RTUStorage::getFunction(r->rtuJack, r->rtuChan, t);

            // т.к. с RTU188 свой обмен
            // mbreg и mbfunc поля не используются
            return true;

        }
        else
        {
            mbcrit << myname << "(initRegInfo): Unknown mbtype='" << dev->dtype
                   << "' for " << it.getProp("name") << endl;
            return false;
        }

        if( mbregFromID )
        {
            if( it.getProp("id").empty() )
                r->mbreg = conf->getSensorID(it.getProp("name"));
            else
                r->mbreg = it.getIntProp("id");
        }
        else
        {
            const string sr( IOBase::initProp(it, "mbreg", prop_prefix, false) );

            if( sr.empty() )
            {
                mbcrit << myname << "(initItem): Unknown 'mbreg' for " << it.getProp("name") << endl;
                return false;
            }

            r->mbreg = ModbusRTU::str2mbData(sr);
        }

        r->mbfunc     = ModbusRTU::fnUnknown;
        const string f( IOBase::initProp(it, "mbfunc", prop_prefix, false) );

        if( !f.empty() )
        {
            r->mbfunc = (ModbusRTU::SlaveFunctionCode)uniset::uni_atoi(f.c_str());

            if( r->mbfunc == ModbusRTU::fnUnknown )
            {
                mbcrit << myname << "(initRegInfo): Unknown mbfunc ='" << f
                       << "' for " << it.getProp("name") << endl;
                return false;
            }
        }

        return true;
    }
    // ------------------------------------------------------------------------------------------
    bool MBConfig::initRTUDevice( std::shared_ptr<RTUDevice>& d, UniXML::iterator& it )
    {
        string mbtype(IOBase::initProp(it, "mbtype", prop_prefix, false));

        if(mbtype.empty())
            mbtype = defaultMBtype;

        d->dtype = getDeviceType(mbtype);

        if( d->dtype == dtUnknown )
        {
            mbcrit << myname << "(initRTUDevice): Unknown tcp_mbtype='" << mbtype << "'"
                   << ". Use: rtu "
                   << " for " << it.getProp("name") << endl;
            return false;
        }

        string addr( IOBase::initProp(it, "mbaddr", prop_prefix, false) );

        if( addr.empty() )
            addr = defaultMBaddr;

        if( addr.empty() )
        {
            mbcrit << myname << "(initRTUDevice): Unknown mbaddr for " << it.getProp("name") << endl;
            return false;
        }

        d->mbaddr = ModbusRTU::str2mbAddr(addr);

        if( d->dtype == MBConfig::dtRTU188 )
        {
            if( !d->rtu188 )
                d->rtu188 = make_shared<RTUStorage>(d->mbaddr);
        }

        return true;
    }
    // ------------------------------------------------------------------------------------------
    bool MBConfig::initItem( UniXML::iterator& it )
    {
        RSProperty p;

        if( !initRSProperty(p, it) )
            return false;

        if( p.t_ai != DefaultObjectId ) // пороговые датчики в список обмена вносить не надо.
            return true;

        string addr( IOBase::initProp(it, "mbaddr", prop_prefix, false) );

        if( addr.empty() )
            addr = defaultMBaddr;

        if( addr.empty() )
        {
            mbcrit << myname << "(initItem): Unknown mbaddr(" << IOBase::initProp(it, "mbaddr", prop_prefix, false) << ")='" << addr << "' for " << it.getProp("name") << endl;
            return false;
        }

        ModbusRTU::ModbusAddr mbaddr = ModbusRTU::str2mbAddr(addr);

        auto dev = addDev(devices, mbaddr, it);

        if( !dev )
        {
            mbcrit << myname << "(initItem): " << it.getProp("name") << " CAN`T ADD for polling!" << endl;
            return false;
        }

        ModbusRTU::ModbusData mbreg = 0;
        int fn = IOBase::initIntProp(it, "mbfunc", prop_prefix, false);

        if( dev->dtype == dtRTU188 )
        {
            auto r_tmp = make_shared<RegInfo>();

            if( !initRTU188item(it, r_tmp) )
            {
                mbcrit << myname << "(initItem): init RTU188 failed for " << it.getProp("name") << endl;
                r_tmp.reset();
                return false;
            }

            mbreg = RTUStorage::getRegister(r_tmp->rtuJack, r_tmp->rtuChan, p.stype);
            fn = RTUStorage::getFunction(r_tmp->rtuJack, r_tmp->rtuChan, p.stype);
        }
        else
        {
            if( mbregFromID )
                mbreg = p.si.id; // conf->getSensorID(it.getProp("name"));
            else
            {
                const string reg( IOBase::initProp(it, "mbreg", prop_prefix, false) );

                if( reg.empty() )
                {
                    mbcrit << myname << "(initItem): unknown mbreg(" << prop_prefix << ") for " << it.getProp("name") << endl;
                    return false;
                }

                mbreg = ModbusRTU::str2mbData(reg);
            }

            if( p.nbit != -1 )
            {
                if( fn == ModbusRTU::fnReadCoilStatus || fn == ModbusRTU::fnReadInputStatus )
                {
                    mbcrit << myname << "(initItem): MISMATCHED CONFIGURATION!  nbit=" << (int)p.nbit << " func=" << fn
                           << " for " << it.getProp("name") << endl;
                    return false;
                }
            }
        }

        /*! приоритет опроса:
         * 1...n - задаёт "частоту" опроса. Т.е. каждые 1...n циклов
        */
        size_t pollfactor = IOBase::initIntProp(it, "pollfactor", prop_prefix, false, 0);

        std::shared_ptr<RegMap> rmap;

        auto rit = dev->pollmap.find(pollfactor);

        if( rit == dev->pollmap.end() )
        {
            rmap = make_shared<RegMap>();
            dev->pollmap.emplace(pollfactor, rmap);
        }
        else
            rmap = rit->second;

        // формула для вычисления ID
        // требования:
        //  - ID > диапазона возможных регитров
        //  - разные функции должны давать разный ID
        ModbusRTU::RegID rID = ModbusRTU::genRegID(mbreg, fn);

        auto ri = addReg(rmap, rID, mbreg, it, dev);

        if( dev->dtype == dtMTR )
        {
            p.rnum = MTR::wsize(ri->mtrType);

            if( p.rnum <= 0 )
            {
                mbcrit << myname << "(initItem): unknown word size for " << it.getProp("name") << endl;
                return false;
            }
        }

        if( !ri )
            return false;

        ri->dev = dev;

        // ПРОВЕРКА!
        // если функция на запись, то надо проверить
        // что один и тотже регистр не перезапишут несколько датчиков
        // это возможно только, если они пишут биты!!
        // ИТОГ:
        // Если для функций записи список датчиков для регистра > 1
        // значит в списке могут быть только битовые датчики
        // и если идёт попытка внести в список не битовый датчик то ОШИБКА!
        // И наоборот: если идёт попытка внести битовый датчик, а в списке
        // уже сидит датчик занимающий целый регистр, то тоже ОШИБКА!
        if( ModbusRTU::isWriteFunction(ri->mbfunc) )
        {
            if( p.nbit < 0 &&  ri->slst.size() > 1 )
            {
                ostringstream sl;
                sl << "[ ";

                for( const auto& i : ri->slst )
                    sl << ORepHelpers::getShortName(conf->oind->getMapName(i.si.id)) << ",";

                sl << "]";

                ostringstream err;
                err << myname << "(initItem): FAILED! Sharing SAVE (not bit saving) to "
                    << " tcp_mbreg=" << ModbusRTU::dat2str(ri->mbreg) << "(" << (int)ri->mbreg << ")"
                    << " conflict with sensors " << sl.str();

                mbcrit  << err.str() << endl;
                throw uniset::SystemError(err.str());
            }

            if( p.nbit >= 0 && ri->slst.size() == 1 )
            {
                auto it2 = ri->slst.begin();

                if( it2->nbit < 0 )
                {
                    ostringstream err;
                    err << myname << "(initItem): FAILED! Sharing SAVE (mbreg="
                        << ModbusRTU::dat2str(ri->mbreg) << "(" << (int)ri->mbreg << ") already used)!"
                        << " IGNORE --> " << it.getProp("name");

                    mbcrit  << err.str() << endl;
                    throw uniset::SystemError(err.str());
                }
            }

            // Раз это регистр для записи, то как минимум надо сперва
            // инициализировать значением из SM
            ri->sm_initOK = IOBase::initIntProp(it, "sm_initOK", prop_prefix, false);
            ri->mb_initOK = true;
        }
        else
        {
            ri->mb_initOK = defaultMBinitOK;
            ri->sm_initOK = false;
        }


        RSProperty* p1 = addProp(ri->slst, std::move(p) );

        if( !p1 )
            return false;

        p1->reg = ri;


        if( p1->rnum > 1 )
        {
            ri->q_count = p1->rnum;
            ri->q_num = 1;

            for( auto i = 1; i < p1->rnum; i++ )
            {
                ModbusRTU::RegID id1 = ModbusRTU::genRegID(mbreg + i, ri->mbfunc);
                auto r = addReg(rmap, id1, mbreg + i, it, dev);
                r->q_num = i + 1;
                r->q_count = 1;
                r->mbfunc = ri->mbfunc;
                r->mb_initOK = defaultMBinitOK;
                r->sm_initOK = false;

                if( ModbusRTU::isWriteFunction(ri->mbfunc) )
                {
                    // Если занимает несколько регистров, а указана функция записи "одного",
                    // то это ошибка..
                    if( ri->mbfunc != ModbusRTU::fnWriteOutputRegisters &&
                            ri->mbfunc != ModbusRTU::fnForceMultipleCoils )
                    {
                        ostringstream err;
                        err << myname << "(initItem): Bad write function ='" << ModbusRTU::fnWriteOutputSingleRegister
                            << "' for vtype='" << p1->vType << "'"
                            << " tcp_mbreg=" << ModbusRTU::dat2str(ri->mbreg)
                            << " for " << it.getProp("name");

                        mbcrit << err.str() << endl;
                        throw uniset::SystemError(err.str());
                    }
                }
            }
        }

        // Фомируем список инициализации
        bool need_init = IOBase::initIntProp(it, "preinit", prop_prefix, false);

        if( need_init && ModbusRTU::isWriteFunction(ri->mbfunc) )
        {
            InitRegInfo ii;
            ii.p = std::move(p);
            ii.dev = dev;
            ii.ri = ri;

            const string s_reg(IOBase::initProp(it, "init_mbreg", prop_prefix, false));

            if( !s_reg.empty() )
                ii.mbreg = ModbusRTU::str2mbData(s_reg);
            else
                ii.mbreg = ri->mbreg;

            string s_mbfunc(it.getProp(prop_prefix + "init_mbfunc"));

            if( !s_mbfunc.empty() )
            {
                ii.mbfunc = (ModbusRTU::SlaveFunctionCode)uniset::uni_atoi(s_mbfunc);

                if( ii.mbfunc == ModbusRTU::fnUnknown )
                {
                    mbcrit << myname << "(initItem): Unknown tcp_init_mbfunc ='" << s_mbfunc
                           << "' for " << it.getProp("name") << endl;
                    return false;
                }
            }
            else
            {
                switch(ri->mbfunc)
                {
                    case ModbusRTU::fnWriteOutputSingleRegister:
                        ii.mbfunc = ModbusRTU::fnReadOutputRegisters;
                        break;

                    case ModbusRTU::fnForceSingleCoil:
                        ii.mbfunc = ModbusRTU::fnReadCoilStatus;
                        break;

                    case ModbusRTU::fnWriteOutputRegisters:
                        ii.mbfunc = ModbusRTU::fnReadOutputRegisters;
                        break;

                    case ModbusRTU::fnForceMultipleCoils:
                        ii.mbfunc = ModbusRTU::fnReadCoilStatus;
                        break;

                    default:
                        ii.mbfunc = ModbusRTU::fnReadOutputRegisters;
                        break;
                }
            }

            initRegList.emplace_back( std::move(ii) );
            ri->mb_initOK = false;
            ri->sm_initOK = false;
        }

        return true;
    }
    // ------------------------------------------------------------------------------------------
    bool MBConfig::initMTRitem( UniXML::iterator& it, std::shared_ptr<RegInfo>& p )
    {
        p->mtrType = MTR::str2type(it.getProp(prop_prefix + "mtrtype"));

        if( p->mtrType == MTR::mtUnknown )
        {
            mbcrit << myname << "(readMTRItem): Unknown mtrtype '"
                   << it.getProp(prop_prefix + "mtrtype")
                   << "' for " << it.getProp("name") << endl;

            return false;
        }

        return true;
    }
    // ------------------------------------------------------------------------------------------
    bool MBConfig::initRTU188item( UniXML::iterator& it, std::shared_ptr<RegInfo>& p )
    {
        const string jack(IOBase::initProp(it, "jack", prop_prefix, false));
        const string chan(IOBase::initProp(it, "channel", prop_prefix, false));

        if( jack.empty() )
        {
            mbcrit << myname << "(readRTU188Item): Unknown " << prop_prefix << "jack='' "
                   << " for " << it.getProp("name") << endl;
            return false;
        }

        p->rtuJack = RTUStorage::s2j(jack);

        if( p->rtuJack == RTUStorage::nUnknown )
        {
            mbcrit << myname << "(readRTU188Item): Unknown " << prop_prefix << "jack=" << jack
                   << " for " << it.getProp("name") << endl;
            return false;
        }

        if( chan.empty() )
        {
            mbcrit << myname << "(readRTU188Item): Unknown channel='' "
                   << " for " << it.getProp("name") << endl;
            return false;
        }

        p->rtuChan = uniset::uni_atoi(chan);

        mblog2 << myname << "(readRTU188Item): add jack='" << jack << "'"
               << " channel='" << p->rtuChan << "'" << endl;

        return true;
    }
    // ------------------------------------------------------------------------------------------
    std::ostream& operator<<( std::ostream& os, const MBConfig::DeviceType& dt )
    {
        switch(dt)
        {
            case MBConfig::dtRTU:
                os << "RTU";
                break;

            case MBConfig::dtMTR:
                os << "MTR";
                break;

            case MBConfig::dtRTU188:
                os << "RTU188";
                break;

            default:
                os << "Unknown device type (" << (int)dt << ")";
                break;
        }

        return os;
    }
    // -----------------------------------------------------------------------------
    std::ostream& operator<<( std::ostream& os, const MBConfig::RSProperty& p )
    {
        os     << " (" << ModbusRTU::dat2str(p.reg->mbreg) << ")"
               << " sid=" << p.si.id
               << " stype=" << p.stype
               << " nbit=" << (int)p.nbit
               << " nbyte=" << p.nbyte
               << " rnum=" << p.rnum
               << " safeval=" << p.safeval
               << " invert=" << p.invert;

        if( p.stype == UniversalIO::AI || p.stype == UniversalIO::AO )
        {
            os     << p.cal
                   << " cdiagram=" << ( p.cdiagram ? "yes" : "no" );
        }

        return os;
    }
    // -----------------------------------------------------------------------------
    void MBConfig::initDeviceList(const std::shared_ptr<UniXML>& xml )
    {
        xmlNode* respNode = 0;

        if( xml )
            respNode = xml->extFindNode(cnode, 1, 1, "DeviceList");

        if( respNode )
        {
            UniXML::iterator it1(respNode);

            if( it1.goChildren() )
            {
                for(; it1.getCurrent(); it1.goNext() )
                {
                    ModbusRTU::ModbusAddr a = ModbusRTU::str2mbAddr(it1.getProp("addr"));
                    initDeviceInfo(devices, a, it1);
                }
            }
            else
                mbwarn << myname << "(init): <DeviceList> empty section..." << endl;
        }
        else
            mbwarn << myname << "(init): <DeviceList> not found..." << endl;
    }
    // -----------------------------------------------------------------------------
    bool MBConfig::initDeviceInfo( RTUDeviceMap& m, ModbusRTU::ModbusAddr a, UniXML::iterator& it )
    {
        auto d = m.find(a);

        if( d == m.end() )
        {
            mbwarn << myname << "(initDeviceInfo): not found device for addr=" << ModbusRTU::addr2str(a) << endl;
            return false;
        }

        auto& dev = d->second;

        dev->ask_every_reg = it.getIntProp("ask_every_reg");

        mbinfo << myname << "(initDeviceInfo): add addr=" << ModbusRTU::addr2str(a)
               << " ask_every_reg=" << dev->ask_every_reg << endl;

        string s(it.getProp("respondSensor"));

        if( !s.empty() )
        {
            dev->resp_id = conf->getSensorID(s);

            if( dev->resp_id == DefaultObjectId )
            {
                mbinfo << myname << "(initDeviceInfo): not found ID for respondSensor=" << s << endl;
                return false;
            }
        }

        const string mod(it.getProp("modeSensor"));

        if( !mod.empty() )
        {
            dev->mode_id = conf->getSensorID(mod);

            if( dev->mode_id == DefaultObjectId )
            {
                mbcrit << myname << "(initDeviceInfo): not found ID for modeSensor=" << mod << endl;
                return false;
            }

            UniversalIO::IOType m_iotype = conf->getIOType(dev->mode_id);

            if( m_iotype != UniversalIO::AI )
            {
                mbcrit << myname << "(initDeviceInfo): modeSensor='" << mod << "' must be 'AI'" << endl;
                return false;
            }
        }

        // сперва проверим не задан ли режим "safemodeResetIfNotRespond"
        if( it.getIntProp("safemodeResetIfNotRespond") )
            dev->safeMode = MBConfig::safeResetIfNotRespond;

        // потом проверим датчик для "safeExternalControl"
        const string safemode = it.getProp("safemodeSensor");

        if( !safemode.empty() )
        {
            dev->safemode_id = conf->getSensorID(safemode);

            if( dev->safemode_id == DefaultObjectId )
            {
                mbcrit << myname << "(initDeviceInfo): not found ID for safemodeSensor=" << safemode << endl;
                return false;
            }

            const string safemodeValue(it.getProp("safemodeValue"));

            if( !safemodeValue.empty() )
                dev->safemode_value = uni_atoi(safemodeValue);

            dev->safeMode = MBConfig::safeExternalControl;
        }

        mbinfo << myname << "(initDeviceInfo): add addr=" << ModbusRTU::addr2str(a) << endl;
        int tout = it.getPIntProp("timeout", default_timeout );

        dev->resp_Delay.set(0, tout);
        dev->resp_invert = it.getIntProp("invert");
        dev->resp_force =  it.getIntProp("force");

        int init_tout = it.getPIntProp("respondInitTimeout", tout);
        dev->resp_ptInit.setTiming(init_tout);

        s = it.getProp("speed");

        if( !s.empty() )
        {
            d->second->speed = ComPort::getSpeed(s);

            if( d->second->speed == ComPort::ComSpeed0 )
            {
                //              d->second->speed = defSpeed;
                mbcrit << myname << "(initDeviceInfo): Unknown speed=" << s <<
                       " for addr=" << ModbusRTU::addr2str(a) << endl;
                return false;
            }
        }

        return true;
    }
    // -----------------------------------------------------------------------------
    std::ostream& operator<<( std::ostream& os, const MBConfig::ExchangeMode& em )
    {
        if( em == MBConfig::emNone )
            return os << "emNone";

        if( em == MBConfig::emWriteOnly )
            return os << "emWriteOnly";

        if( em == MBConfig::emReadOnly )
            return os << "emReadOnly";

        if( em == MBConfig::emSkipSaveToSM )
            return os << "emSkipSaveToSM";

        if( em == MBConfig::emSkipExchange )
            return os << "emSkipExchange";

        return os;
    }
    // -----------------------------------------------------------------------------
    std::string to_string( const MBConfig::SafeMode& m )
    {
        if( m == MBConfig::safeNone )
            return "safeNone";

        if( m == MBConfig::safeResetIfNotRespond )
            return "safeResetIfNotRespond";

        if( m == MBConfig::safeExternalControl )
            return "safeExternalControl";

        return "";
    }

    ostream& operator<<( ostream& os, const MBConfig::SafeMode& m )
    {
        return os << to_string(m);
    }
    // -----------------------------------------------------------------------------
    bool MBConfig::RTUDevice::checkRespond( std::shared_ptr<DebugStream>& mblog )
    {
        bool prev = resp_state;
        resp_state = resp_Delay.check( prev_numreply != numreply ) && numreply != 0;

        mblog4 << "(checkRespond): addr=" << ModbusRTU::addr2str(mbaddr)
               << " respond_id=" << resp_id
               << " state=" << resp_state
               << " check=" << (prev_numreply != numreply)
               << " delay_check=" << resp_Delay.get()
               << " [ timeout=" << resp_Delay.getOffDelay()
               << " numreply=" << numreply
               << " prev_numreply=" << prev_numreply
               << " resp_ptInit=" << resp_ptInit.checkTime()
               << " ]"
               << endl;

        // если только что прошла "инициализация" возвращаем true
        // чтобы датчик в SM обновился..
        if( trInitOK.hi(resp_ptInit.checkTime()) )
            return true;

        return ((prev != resp_state || resp_force ) && trInitOK.get() );
    }
    // -----------------------------------------------------------------------------
    std::string MBConfig::RTUDevice::getShortInfo() const
    {
        size_t regs = 0;

        for( const auto& p : pollmap )
            regs += p.second->size();

        ostringstream s;

        s << "mbaddr=" << ModbusRTU::addr2str(mbaddr) << ":"
          << " resp_state=" << resp_state
          << " (resp_id=" << resp_id << " resp_force=" << resp_force
          << " resp_invert=" << resp_invert
          << " numreply=" << numreply
          << " timeout=" << resp_Delay.getOffDelay()
          << " type=" << dtype
          << " ask_every_reg=" << ask_every_reg
          << " regs=" << regs
          << ")" << endl;
        return s.str();
    }
    // -----------------------------------------------------------------------------
    std::string MBConfig::getShortInfo() const
    {
        ostringstream s;
        s << " recv_timeout=" << recv_timeout
          << " default_timeout=" << default_timeout
          << " aftersend_pause=" << aftersend_pause
          << " polltime=" << polltime
          << " sleepPause_msec=" << sleepPause_msec
          << " maxQueryCount=" << maxQueryCount
          << " noQueryOptimization=" << noQueryOptimization
          << " prefix=" << prefix
          << " prop_prefix=" << prop_prefix
          << " s_field=" << s_field
          << " s_fvalue=" << s_fvalue
          << " defaultMBtype=" << defaultMBtype
          << " defaultMBaddr=" << defaultMBaddr
          << " mbregFromID=" << mbregFromID
          << " defaultMBinitOK=" << defaultMBinitOK;

        return s.str();
    }
    // -----------------------------------------------------------------------------
} // end of namespace uniset
