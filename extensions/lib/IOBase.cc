#include "Configuration.h"
#include "Extensions.h"
#include "IOBase.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, IOBase& inf )
{
    return os << "(" << inf.si.id << ")" << conf->oind->getMapName(inf.si.id)
        << " default=" << inf.defval << " safety=" << inf.safety;
}
// -----------------------------------------------------------------------------
bool IOBase::check_channel_break( long val )
{
    // порог не задан... (проверка отключена)
    if( breaklim <= 0 )
        return false;

    return ( val < breaklim );
}
// -----------------------------------------------------------------------------
bool IOBase::check_debounce( bool val )
{
    // нет защиты от дребезга
    if( ptDebounce.getInterval() <= 0 )
    {
        debounce_state = val;
        return val;
    }

    if( trdebounce.change(val) )
    {
        if( !debounce_pause )
        {
            // засекаем время...
            debounce_pause = true;
            ptDebounce.reset();
        }
    }

    if( debounce_pause && ptDebounce.checkTime() )
    {
        // пауза на дребезг кончилась
        // сохраняем значение
        debounce_state = val;
        debounce_pause = false;
    }

    // возвращаем ТЕКУЩЕЕ, А НЕ НОВОЕ значение
    return debounce_state;
}
// -----------------------------------------------------------------------------
bool IOBase::check_on_delay( bool val )
{
    // нет задержки на включение
    if( ptOnDelay.getInterval() <= 0 )
    {
        ondelay_state = val;
        return val;
    }

    if( trOnDelay.hi(val) )
        ptOnDelay.reset();

    // обновляем значение только если наступило время
    // или если оно "0"...
    if( !val || (val && ptOnDelay.checkTime()) )
        ondelay_state = val;

    // возвращаем ТЕКУЩЕЕ, А НЕ НОВОЕ значение
    return ondelay_state;
}
// -----------------------------------------------------------------------------
bool IOBase::check_off_delay( bool val )
{
    if( ptOffDelay.getInterval() <= 0 )
    {
        offdelay_state = val;
        return val;
    }

    if( trOffDelay.low(val) )
        ptOffDelay.reset();

    // обновляем значение только если наступило время
    // или если оно "1"...
    if( val || (!val && ptOffDelay.checkTime()) )
        offdelay_state = val;

    // возвращаем ТЕКУЩЕЕ, А НЕ НОВОЕ значение
    return offdelay_state;
}
// -----------------------------------------------------------------------------
bool IOBase::check_front( bool val )
{
    if( !front || front_type == ftUnknown )
        return val;

    if( front_type == ft01 )
    {
        if( val && !front_prev_state )
            front_state ^= true;
    }
    else if( front_type == ft10 )
    {
        if( !val && front_prev_state )
            front_state ^= true;
    }

    front_prev_state = val;
    return front_state;
}
// -----------------------------------------------------------------------------
void IOBase::processingAsAI( IOBase* it, long val, SMInterface* shm, bool force )
{
    // проверка на обрыв
    if( it->check_channel_break(val) )
    {
        uniset_rwmutex_wrlock lock(it->val_lock);
        it->value = ChannelBreakValue;
        shm->localSetUndefinedState(it->ioit,true,it->si.id);
        return;
    }

    // проверка зависимости
        if( !it->nofilter && it->df.size() > 1 )
        {
            if( it->f_median )
                val = it->df.median(val);
            else if( it->f_filter_iir )
                val = it->df.filterIIR(val);
            else if( it->f_ls )
                val = it->df.leastsqr(val);
            else
                val = it->df.filterRC(val);
        }

        if( it->cdiagram )    // задана специальная калибровочная диаграмма
        {
            if( it->craw != val )
            {
                it->craw = val;
                val = it->cdiagram->getValue(val);
                it->cprev = val;
            }
            else
                val = it->cprev;        // просто передаём предыдущее значение
        }
        else
        {
            IOController_i::CalibrateInfo* cal( &(it->cal) );
            if( cal->maxRaw!=cal->minRaw ) // задана обычная калибровка
                val = UniSetTypes::lcalibrate(val,cal->minRaw,cal->maxRaw,cal->minCal,cal->maxCal,true);
        }

        if( !it->noprecision && it->cal.precision > 0 )
            val *= lround(pow10(it->cal.precision));

    // если предыдущее значение "обрыв",
    // то сбрасываем признак
    {
        uniset_rwmutex_wrlock lock(it->val_lock);
        if( it->value == ChannelBreakValue )
            shm->localSetUndefinedState(it->ioit,false,it->si.id);

        if( force || it->value != val )
        {
            shm->localSetValue( it->ioit,it->si.id,val,shm->ID() );
            it->value = val;
        }
    }
}
// -----------------------------------------------------------------------------
void IOBase::processingFasAI( IOBase* it, float fval, SMInterface* shm, bool force )
{
    long val = lroundf(fval);

    if( it->cal.precision > 0 )
        val = lroundf( fval * pow10(it->cal.precision) );

    // проверка на обрыв
    if( it->check_channel_break(val) )
    {
        uniset_rwmutex_wrlock lock(it->val_lock);
        it->value = ChannelBreakValue;
        shm->localSetUndefinedState(it->ioit,true,it->si.id);
        return;
    }

    // проверка зависимости
    {
        // Читаем с использованием фильтра...
        if( !it->nofilter )
        {
            if( it->df.size() > 1 )
                it->df.add(val);

            val = it->df.filterRC(val);
        }

        IOController_i::CalibrateInfo* cal( &(it->cal) );
        if( cal->maxRaw!=cal->minRaw ) // задана обычная калибровка
            val = UniSetTypes::lcalibrate(val,cal->minRaw,cal->maxRaw,cal->minCal,cal->maxCal,true);
    }

    // если предыдущее значение "обрыв",
    // то сбрасываем признак
    {
        uniset_rwmutex_wrlock lock(it->val_lock);
        if( it->value == ChannelBreakValue )
            shm->localSetUndefinedState(it->ioit,false,it->si.id);

        if( force || it->value != val )
        {
            shm->localSetValue( it->ioit,it->si.id,val,shm->ID() );
            it->value = val;
        }
    }
}
// -----------------------------------------------------------------------------
void IOBase::processingAsDI( IOBase* it, bool set, SMInterface* shm, bool force )
{
    // проверка зависимости
    if( it->invert )
        set ^= true;

    // Проверяем именно в такой последовательности!
    set = it->check_debounce(set);       // фильтр дребезга
    set = it->check_on_delay(set);  // фильтр на срабатывание
    set = it->check_off_delay(set); // фильтр на отпускание
    set = it->check_front(set);     // работа по фронту (проверять после debounce_xxx!)

    {
        uniset_rwmutex_wrlock lock(it->val_lock);
        if( force || (bool)it->value!=set )
        {
            shm->localSetValue( it->ioit,it->si.id,(set ? 1:0),shm->ID() );
            it->value = set ? 1 : 0;
        }
    }
}
// -----------------------------------------------------------------------------
long IOBase::processingAsAO( IOBase* it, SMInterface* shm, bool force )
{
    uniset_rwmutex_rlock lock(it->val_lock);
    long val = it->value;

    if( force )
    {
        val = shm->localGetValue(it->ioit,it->si.id);
        it->value = val;
    }

    if( it->stype == UniversalIO::AO ||
        it->stype == UniversalIO::AI )
    {
        if( it->cdiagram )    // задана специальная калибровочная диаграмма
        {
            if( it->cprev != it->value )
            {
                it->cprev = it->value;
                val = it->cdiagram->getRawValue(val);
                it->craw = val;
            }
            else
                val = it->craw; // просто передаём предыдущее значение
        }
        else
        {
            IOController_i::CalibrateInfo* cal( &(it->cal) );
            if( cal && cal->maxRaw!=cal->minRaw ) // задана калибровка
            {
                // Калибруем в обратную сторону!!!
                val = UniSetTypes::lcalibrate(it->value,
                            cal->minCal, cal->maxCal, cal->minRaw, cal->maxRaw, true );
            }
            else
                val = it->value;

            if( !it->noprecision && it->cal.precision > 0 )
                return lroundf( (float)it->value / pow10(it->cal.precision) );
        }
    }

    return val;
}
// -----------------------------------------------------------------------------
bool IOBase::processingAsDO( IOBase* it, SMInterface* shm, bool force )
{
    uniset_rwmutex_rlock lock(it->val_lock);
    bool set = it->value;

    if( force )
        set = shm->localGetValue(it->ioit,it->si.id) ? true : false;

    set = it->invert ? !set : set;
    return set;
}
// -----------------------------------------------------------------------------
float IOBase::processingFasAO( IOBase* it, SMInterface* shm, bool force )
{
    uniset_rwmutex_rlock lock(it->val_lock);
    long val = it->value;

    if( force )
    {
        val = shm->localGetValue(it->ioit,it->si.id);
        it->value = val;
    }

    if( it->stype == UniversalIO::AO ||
        it->stype == UniversalIO::AI )
    {
        if( it->cdiagram )    // задана специальная калибровочная диаграмма
        {
            if( it->cprev != it->value )
            {
                it->cprev = it->value;
                val = it->cdiagram->getRawValue(val);
                it->craw = val;
            }
            else
                val = it->craw; // просто передаём предыдущее значение
        }
        else
        {
            float fval = val;
            IOController_i::CalibrateInfo* cal( &(it->cal) );
            if( cal->maxRaw!=cal->minRaw ) // задана калибровка
            {
                // Калибруем в обратную сторону!!!
                fval = UniSetTypes::fcalibrate(fval,
                            cal->minCal, cal->maxCal, cal->minRaw, cal->maxRaw, true );
            }

            if( !it->noprecision && it->cal.precision > 0 )
                return ( fval / pow10(it->cal.precision) );
        }
    }

    return val;
}
// -----------------------------------------------------------------------------
void IOBase::processingThreshold( IOBase* it, SMInterface* shm, bool force )
{
    if( it->t_ai == DefaultObjectId )
        return;

    long val = shm->localGetValue(it->t_ait,it->t_ai);
    bool set = it->value ? true : false;

//    cout  << "val=" << val << " set=" << set << endl;
    // Проверка нижнего предела
    // значение должно быть меньше lowLimit-чуствительность
    if (it->ti.invert)
    {
        if( val <= it->ti.lowlimit )
            set = true;
        else if( val >= it->ti.hilimit )
            set = false;
    }
    else
    {
        if( val <= it->ti.lowlimit )
            set = false;
        else if( val >= it->ti.hilimit )
            set = true;
    }

//    cout  << "thresh: set=" << set << endl;
    processingAsDI(it,set,shm,force);
}
// -----------------------------------------------------------------------------
std::string IOBase::initProp( UniXML_iterator& it, const std::string& prop, const std::string& prefix, bool prefonly, const std::string& defval )
{
	if( !it.getProp(prefix+prop).empty() )
		return it.getProp(prefix+prop);

	if( prefonly )
		return defval;

	if( !it.getProp(prop).empty() )
		return it.getProp(prop);

	return defval;
}
// -----------------------------------------------------------------------------
int IOBase::initIntProp( UniXML_iterator& it, const std::string& prop, const std::string& prefix, bool prefonly, const int defval )
{
	string pp(prefix+prop);

	if( !it.getProp(pp).empty() )
		return it.getIntProp(pp);
	
	if( prefonly )
		return defval;

	if( !it.getProp(prop).empty() )
		return it.getIntProp(prop);

	return defval;
}
// -----------------------------------------------------------------------------
bool IOBase::initItem( IOBase* b, UniXML_iterator& it, SMInterface* shm, const std::string& prefix, 
                        bool init_prefix_only,
                        DebugStream* dlog, std::string myname,
                        int def_filtersize, float def_filterT, float def_lsparam,
                        float def_iir_coeff_prev, float def_iir_coeff_new )
{
	// Переопределять ID и name - нельзя..s
    string sname( it.getProp("name") );

    ObjectId sid = DefaultObjectId;
    if( it.getProp("id").empty() )
        sid = conf->getSensorID(sname);
    else
        sid = it.getPIntProp("id",DefaultObjectId);

    if( sid == DefaultObjectId )
    {
        if( dlog && dlog->is_crit() )
            dlog->crit() << myname << "(readItem): (" << DefaultObjectId << ") Не удалось получить ID для датчика: "
                        << sname << endl;
        return false;
    }

    b->val_lock.setName(sname + "_lock");

    b->si.id     = sid;
    b->si.node     = conf->getLocalNode();

    b->nofilter = initIntProp(it,"nofilter",prefix,init_prefix_only);
    b->ignore    = initIntProp(it,"ioignore",prefix,init_prefix_only);
    b->invert    = initIntProp(it,"ioinvert",prefix,init_prefix_only);
    b->defval     = initIntProp(it,"default",prefix,init_prefix_only);
    b->noprecision    = initIntProp(it,"noprecision",prefix,init_prefix_only);
    b->value    = b->defval;
    b->breaklim = initIntProp(it,"breaklim",prefix,init_prefix_only);

    long msec = initIntProp(it,"debouncedelay",prefix,init_prefix_only, UniSetTimer::WaitUpTime);
    b->ptDebounce.setTiming(msec);

    msec = initIntProp(it,"ondelay",prefix,init_prefix_only, UniSetTimer::WaitUpTime);
    b->ptOnDelay.setTiming(msec);

    msec = initIntProp(it,"offdelay",prefix,init_prefix_only, UniSetTimer::WaitUpTime);
    b->ptOffDelay.setTiming(msec);


    b->front = false;
    std::string front_t( initProp(it,"iofront",prefix,init_prefix_only) );
    if( !front_t.empty() )
    {
        if( front_t == "01" )
        {
            b->front = true;
            b->front_type = ft01;
        }
        else if( front_t == "10" )
        {
            b->front = true;
            b->front_type = ft10;
        }
        else
        {
            if( dlog && dlog->is_crit() )
                dlog->crit() << myname << "(IOBase::readItem): Unknown iofront='" << front_t << "'"
                    << " for '" << sname << "'.  Must be [ 01, 10 ]." << endl;
            return false;
        }
    }

    b->safety = initIntProp(it,"safety",prefix,init_prefix_only, NoSafety);

    b->stype = UniSetTypes::getIOType(initProp(it,"iotype",prefix,init_prefix_only));
    if( b->stype == UniversalIO::UnknownIOType )
    {
        if( dlog && dlog->is_crit() )
            dlog->crit() << myname << "(IOBase::readItem): неизвестный iotype=: "
                << initProp(it,"iotype",prefix,init_prefix_only) << " для " << sname << endl;
        return false;
    }

    b->cal.minRaw = 0;
    b->cal.maxRaw = 0;
    b->cal.minCal = 0;
    b->cal.maxCal = 0;
    b->cal.precision = 0;
    b->cdiagram = 0;
    b->f_median = false;
    b->f_ls = false;
    b->f_filter_iir = false;

    shm->initIterator(b->ioit);

    if( b->stype == UniversalIO::AI || b->stype == UniversalIO::AO )
    {
        b->cal.minRaw = initIntProp(it,"rmin",prefix,init_prefix_only);
        b->cal.maxRaw = initIntProp(it,"rmax",prefix,init_prefix_only);
        b->cal.minCal = initIntProp(it,"cmin",prefix,init_prefix_only);
        b->cal.maxCal = initIntProp(it,"cmax",prefix,init_prefix_only);
        b->cal.precision = initIntProp(it,"precision",prefix,init_prefix_only);

        int f_size     = def_filtersize;
        float f_T     = def_filterT;
        float f_lsparam = def_lsparam;
        int f_median = initIntProp(it,"filtermedian",prefix,init_prefix_only);
        int f_iir = initIntProp(it,"iir_thr",prefix,init_prefix_only);
        float f_iir_coeff_prev = def_iir_coeff_prev;
        float f_iir_coeff_new = def_iir_coeff_new;

        if( f_median > 0 )
        {
            f_size = f_median;
            b->f_median = true;
        }
        else
        {
            if( f_iir > 0 )
                b->f_filter_iir = true;
            if( !initProp(it,"filtersize",prefix,init_prefix_only).empty() )
                f_size = initIntProp(it,"filtersize",prefix,init_prefix_only,def_filtersize);
        }

        if( !initProp(it,"filterT",prefix,init_prefix_only).empty() )
        {
            f_T = atof(initProp(it,"filterT",prefix,init_prefix_only).c_str());
            if( f_T < 0 )
                f_T = 0.0;
        }

        if( !initProp(it,"leastsqr",prefix,init_prefix_only).empty() )
        {
            b->f_ls = true;
            f_lsparam = atof(initProp(it,"leastsqr",prefix,init_prefix_only).c_str());
            if( f_lsparam < 0 )
                f_lsparam = def_lsparam;
        }

        if( !initProp(it,"iir_coeff_prev",prefix,init_prefix_only).empty() )
            f_iir_coeff_prev = atof(initProp(it,"iir_coeff_prev",prefix,init_prefix_only).c_str());
        if( !initProp(it,"iir_coeff_new",prefix,init_prefix_only).empty() )
            f_iir_coeff_new = atof(initProp(it,"iir_coeff_new",prefix,init_prefix_only).c_str());

        if( b->stype == UniversalIO::AI )
            b->df.setSettings( f_size, f_T, f_lsparam, f_iir,
                               f_iir_coeff_prev, f_iir_coeff_new );

        b->df.init(b->defval);

        std::string caldiagram( initProp(it,"caldiagram",prefix,init_prefix_only) );
        if( !caldiagram.empty() )
        {
            b->cdiagram = UniSetExtensions::buildCalibrationDiagram(caldiagram);
            if( !initProp(it,"cal_cachesize",prefix,init_prefix_only).empty() )
                b->cdiagram->setCacheSize(initIntProp(it,"cal_cachesize",prefix,init_prefix_only));
            if( !initProp(it,"cal_cacheresort",prefix,init_prefix_only).empty() )
                b->cdiagram->setCacheResortCycle(initIntProp(it,"cal_cacheresort",prefix,init_prefix_only));
        }
    }
    else if( b->stype == UniversalIO::DI || b->stype == UniversalIO::DO )
    {
        string tai(initProp(it,"threshold_aid",prefix,init_prefix_only));
        if( !tai.empty() )
        {
            b->t_ai = conf->getSensorID(tai);
            if( b->t_ai == DefaultObjectId )
            {
                if( dlog && dlog->is_crit() )
                    dlog->crit() << myname << "(IOBase::readItem): unknown ID for threshold_ai "
                        << tai << endl;
                return false;
            }

            b->ti.lowlimit = initIntProp(it,"lowlimit",prefix,init_prefix_only);
            b->ti.hilimit = initIntProp(it,"hilimit",prefix,init_prefix_only);
            b->ti.invert = initIntProp(it,"threshold_invert",prefix,init_prefix_only);
            shm->initIterator(b->t_ait);
        }
    }

    return true;
}
// -----------------------------------------------------------------------------
