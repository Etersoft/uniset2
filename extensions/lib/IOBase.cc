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
bool IOBase::check_jar( bool val )
{
	// нет защиты от дребезга
	if( ptJar.getInterval() <= 0 )
	{
		jar_state = val;
		return val;
	}

	if( trJar.change(val) )
	{
		if( !jar_pause )
		{	
			// засекаем время...
			jar_pause = true;
			ptJar.reset();
		}
	}
	
	if( jar_pause && ptJar.checkTime() )
	{
		// пауза на дребезг кончилась
		// сохраняем значение
		jar_state = val;
		jar_pause = false;
	}

	// возвращаем ТЕКУЩЕЕ, А НЕ НОВОЕ значение
	return jar_state;
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
bool IOBase::check_depend( SMInterface* shm )
{
	if( d_id == DefaultObjectId )
		return true;

	if( d_iotype == UniversalIO::DigitalInput || d_iotype == UniversalIO::DigitalOutput  )
	{
		if( shm->localGetState(d_dit,d_id) == (bool)d_value )
			return true;

		return false;
	}

	if( d_iotype == UniversalIO::AnalogInput || d_iotype == UniversalIO::AnalogOutput )
	{
		if( shm->localGetValue(d_ait,d_id) == d_value )
			return true;

		return false;
	}

	return true;
}
// -----------------------------------------------------------------------------
void IOBase::processingAsAI( IOBase* it, long val, SMInterface* shm, bool force )
{
	// проверка на обрыв
	if( it->check_channel_break(val) )
	{
		uniset_rwmutex_wrlock lock(it->val_lock);
		it->value = ChannelBreakValue;
		shm->localSetUndefinedState(it->ait,true,it->si.id);
		return;
	}

	// проверка зависимости
	if( !it->check_depend(shm) )
		val = it->d_off_value;
	else
	{
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

		if( it->cdiagram )	// задана специальная калибровочная диаграмма
		{
			if( it->craw != val )
			{
				it->craw = val;
				val = it->cdiagram->getValue(val);
				it->cprev = val;
			}
			else
				val = it->cprev;		// просто передаём предыдущее значение
		}
		else
		{
			IOController_i::CalibrateInfo* cal( &(it->cal) );
			if( cal->maxRaw!=cal->minRaw ) // задана обычная калибровка
				val = UniSetTypes::lcalibrate(val,cal->minRaw,cal->maxRaw,cal->minCal,cal->maxCal,true);
		}

		if( !it->noprecision && it->cal.precision > 0 )
			val *= lround(pow10(it->cal.precision));

	} // end of 'check_depend'

	// если предыдущее значение "обрыв",
	// то сбрасываем признак 
	{
		uniset_rwmutex_wrlock lock(it->val_lock);
		if( it->value == ChannelBreakValue )
			shm->localSetUndefinedState(it->ait,false,it->si.id);

		if( force || it->value != val )
		{
			if( it->stype == UniversalIO::AnalogInput )
				shm->localSaveValue( it->ait,it->si.id,val,shm->ID() );
			else if( it->stype == UniversalIO::AnalogOutput )
				shm->localSetValue( it->ait,it->si.id,val,shm->ID() );
			else if( it->stype == UniversalIO::DigitalOutput )
				shm->localSetState( it->dit,it->si.id,(bool)val,shm->ID() );
			else if( it->stype == UniversalIO::DigitalInput )
				shm->localSaveState( it->dit,it->si.id,(bool)val,shm->ID() );

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
		shm->localSetUndefinedState(it->ait,true,it->si.id);
		return;
	}

	// проверка зависимости
	if( !it->check_depend(shm) )
		val = it->d_off_value;
	else
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
			shm->localSetUndefinedState(it->ait,false,it->si.id);

		if( force || it->value != val )
		{
			if( it->stype == UniversalIO::AnalogInput )
				shm->localSaveValue( it->ait,it->si.id,val,shm->ID() );
			else if( it->stype == UniversalIO::AnalogOutput )
				shm->localSetValue( it->ait,it->si.id,val,shm->ID() );
			else if( it->stype == UniversalIO::DigitalOutput )
				shm->localSetState( it->dit,it->si.id,(bool)val,shm->ID() );
			else if( it->stype == UniversalIO::DigitalInput )
				shm->localSaveState( it->dit,it->si.id,(bool)val,shm->ID() );

			it->value = val;
		}
	}
}
// -----------------------------------------------------------------------------
void IOBase::processingAsDI( IOBase* it, bool set, SMInterface* shm, bool force )
{
	// проверка зависимости
	if( !it->check_depend(shm) )
		set = (bool)it->d_off_value;
	else if( it->invert )
		set ^= true;

	// Проверяем именно в такой последовательности!
	set = it->check_jar(set);       // фильтр дребезга
	set = it->check_on_delay(set);  // фильтр на срабатывание
	set = it->check_off_delay(set); // фильтр на отпускание

	{
		uniset_rwmutex_wrlock lock(it->val_lock);
		if( force || (bool)it->value!=set )
		{
			if( it->stype == UniversalIO::DigitalInput )
				shm->localSaveState(it->dit,it->si.id,set,shm->ID());
			else if( it->stype == UniversalIO::DigitalOutput )
				shm->localSetState(it->dit,it->si.id,set,shm->ID());
			else if( it->stype == UniversalIO::AnalogInput )
				shm->localSaveValue( it->ait,it->si.id,(set ? 1:0),shm->ID() );
			else if( it->stype == UniversalIO::AnalogOutput )
				shm->localSetValue( it->ait,it->si.id,(set ? 1:0),shm->ID() );
			
			it->value = set ? 1 : 0;
		}
	}
}
// -----------------------------------------------------------------------------
long IOBase::processingAsAO( IOBase* it, SMInterface* shm, bool force )
{
	uniset_rwmutex_rlock lock(it->val_lock);
	long val = it->value;
	
	// проверка зависимости
	if( !it->check_depend(shm) )
		return it->d_off_value;

	if( force )
	{
		if( it->stype == UniversalIO::DigitalInput || it->stype == UniversalIO::DigitalOutput )
			val = shm->localGetState(it->dit,it->si.id) ? 1 : 0;
		else if( it->stype == UniversalIO::AnalogInput || it->stype == UniversalIO::AnalogOutput )
			val = shm->localGetValue(it->ait,it->si.id);

		it->value = val;
	}

	if( it->stype == UniversalIO::AnalogOutput ||
		it->stype == UniversalIO::AnalogInput )
	{
		if( it->cdiagram )	// задана специальная калибровочная диаграмма
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
	// проверка зависимости
	if( !it->check_depend(shm) )
		return (bool)it->d_off_value;

	uniset_rwmutex_rlock lock(it->val_lock);
	bool set = it->value;

	if( force )
	{
		if( it->stype == UniversalIO::DigitalInput || it->stype == UniversalIO::DigitalOutput )
			set = shm->localGetState(it->dit,it->si.id);
		else if( it->stype == UniversalIO::AnalogInput || it->stype == UniversalIO::AnalogOutput )
			set = shm->localGetValue(it->ait,it->si.id) ? true : false;
	}
		
	set = it->invert ? !set : set;
	return set;
}
// -----------------------------------------------------------------------------
float IOBase::processingFasAO( IOBase* it, SMInterface* shm, bool force )
{
	// проверка зависимости
	if( !it->check_depend(shm) )
		return (float)it->d_off_value;

	uniset_rwmutex_rlock lock(it->val_lock);
	long val = it->value;
	
	if( force )
	{
		val = shm->localGetValue(it->ait,it->si.id);
		it->value = val;
	}

	if( it->stype == UniversalIO::AnalogOutput ||
		it->stype == UniversalIO::AnalogInput )
	{
		if( it->cdiagram )	// задана специальная калибровочная диаграмма
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
	
	long val = shm->localGetValue(it->ait,it->t_ai);
	bool set = it->value ? true : false;

//	cout  << "val=" << val << " set=" << set << endl;
	// Проверка нижнего предела
	// значение должно быть меньше lowLimit-чуствительность
	if (it->ti.inverse)
	{
		if( val <= (it->ti.lowlimit-it->ti.sensibility) )
			set = true;
		else if( val >= (it->ti.hilimit+it->ti.sensibility) )
			set = false;
	}
	else
	{
		if( val <= (it->ti.lowlimit-it->ti.sensibility) )
			set = false;
		else if( val >= (it->ti.hilimit+it->ti.sensibility) )
			set = true;
	}

//	cout  << "thresh: set=" << set << endl;
	processingAsDI(it,set,shm,force);
}
// -----------------------------------------------------------------------------

bool IOBase::initItem( IOBase* b, UniXML_iterator& it, SMInterface* shm,  
						DebugStream* dlog, std::string myname,
						int def_filtersize, float def_filterT, float def_lsparam,
						float def_iir_coeff_prev, float def_iir_coeff_new )
{
	string sname( it.getProp("name") );

	ObjectId sid = DefaultObjectId;
	if( it.getProp("id").empty() )
		sid = conf->getSensorID(sname);
	else
	{
		sid = it.getPIntProp("id", DefaultObjectId);
	}
	
	if( sid == DefaultObjectId )
	{
		if( dlog )
			dlog[Debug::CRIT] << myname << "(readItem): (-1) Не удалось получить ID для датчика: "
						<< sname << endl;
		return false;
	}

	b->si.id 	= sid;
	b->si.node 	= conf->getLocalNode();

	b->nofilter = it.getIntProp("nofilter");
	b->ignore	= it.getIntProp("ioignore");
	b->invert	= it.getIntProp("ioinvert");
	b->defval 	= it.getIntProp("default");
	b->noprecision	= it.getIntProp("noprecision");
	b->value	= b->defval;
	b->breaklim = it.getIntProp("breaklim");
	
	long msec = it.getPIntProp("jardelay", UniSetTimer::WaitUpTime);
	b->ptJar.setTiming(msec);

	msec = it.getPIntProp("ondelay", UniSetTimer::WaitUpTime);
	b->ptOnDelay.setTiming(msec);

	msec = it.getPIntProp("offdelay", UniSetTimer::WaitUpTime);
	b->ptOffDelay.setTiming(msec);
	
	b->safety = it.getPIntProp("safety", NoSafety);

	b->stype = UniSetTypes::getIOType(it.getProp("iotype"));
	if( b->stype == UniversalIO::UnknownIOType )
	{
		if( dlog )
			dlog[Debug::CRIT] << myname << "(IOBase::readItem): неизвестный iotype=: " 
				<< it.getProp("iotype") << " для " << sname << endl;
		return false;
	}

	b->cal.minRaw = 0;
	b->cal.maxRaw = 0;
	b->cal.minCal = 0;
	b->cal.maxCal = 0;
	b->cal.sensibility = 0;
	b->cal.precision = 0;
	b->cdiagram = 0;
	b->f_median = false;
	b->f_ls = false;
	b->f_filter_iir = false;
		
	shm->initAIterator(b->ait);
	shm->initAIterator(b->d_ait);
	shm->initDIterator(b->dit);
	shm->initDIterator(b->d_dit);

	string d_txt(it.getProp("depend"));
	if( !d_txt.empty() )
	{
		b->d_id = conf->getSensorID(d_txt);
		if( b->d_id == DefaultObjectId )
		{
			if( dlog )
				dlog[Debug::CRIT] << myname << "(IOBase::readItem): sensor='"
					<< it.getProp("name") << "' err: "
					<< " Unknown SensorID for depend='"  << d_txt
					<< endl;
			return false;
		}

		// по умолчанию срабатывание на "1"
		b->d_value = it.getProp("depend_value").empty() ? 1 : it.getIntProp("depend_value");
		b->d_off_value = it.getIntProp("depend_off_value");
		b->d_iotype = conf->getIOType(b->d_id);
	}

	if( b->stype == UniversalIO::AnalogInput || b->stype == UniversalIO::AnalogOutput )
	{
		b->cal.minRaw = it.getIntProp("rmin");
		b->cal.maxRaw = it.getIntProp("rmax");
		b->cal.minCal = it.getIntProp("cmin");
		b->cal.maxCal = it.getIntProp("cmax");
		b->cal.sensibility = it.getIntProp("sensibility");
		b->cal.precision = it.getIntProp("precision");

		int f_size 	= def_filtersize;
		float f_T 	= def_filterT;
		float f_lsparam = def_lsparam;
		int f_median = it.getIntProp("filtermedian");
		int f_iir = it.getIntProp("iir_thr");
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
			if( !it.getProp("filtersize").empty() )
				f_size = it.getPIntProp("filtersize",def_filtersize);
		}
		
		if( !it.getProp("filterT").empty() )
		{
			f_T = atof(it.getProp("filterT").c_str());
			if( f_T < 0 )
				f_T = 0.0;
		}

		if( !it.getProp("leastsqr").empty() )
		{
			b->f_ls = true;
			f_lsparam = atof(it.getProp("leastsqr").c_str());
			if( f_lsparam < 0 )
				f_lsparam = def_lsparam;
		}

		if( !it.getProp("iir_coeff_prev").empty() )
			f_iir_coeff_prev = atof(it.getProp("iir_coeff_prev").c_str());
		if( !it.getProp("iir_coeff_new").empty() )
			f_iir_coeff_new = atof(it.getProp("iir_coeff_new").c_str());

		if( b->stype == UniversalIO::AnalogInput )
			b->df.setSettings( f_size, f_T, f_lsparam, f_iir,
			                   f_iir_coeff_prev, f_iir_coeff_new );

		b->df.init(b->defval);

		std::string caldiagram( it.getProp("caldiagram") );
		if( !caldiagram.empty() )
			b->cdiagram = UniSetExtensions::buildCalibrationDiagram(caldiagram);
	}
	else if( b->stype == UniversalIO::DigitalInput || b->stype == UniversalIO::DigitalOutput )
	{
		string tai(it.getProp("threshold_aid"));
		if( !tai.empty() )
		{
			b->t_ai = conf->getSensorID(tai);
			if( b->t_ai == DefaultObjectId )
			{
				if( dlog )
					dlog[Debug::CRIT] << myname << "(IOBase::readItem): unknown ID for threshold_ai "
						<< tai << endl;
				return false;
			}
		
			b->ti.lowlimit 	= it.getIntProp("lowlimit");
			b->ti.hilimit 		= it.getIntProp("hilimit");
			b->ti.sensibility 	= it.getIntProp("sensibility");
			b->ti.inverse 		= it.getIntProp("inverse");
		}
	}
//	else
//	{
//		dlog[Debug::CRIT] << myname << "(IOBase::readItem): неизвестный iotype=: " << stype << " для " << sname << endl;
//		return false;
//	}

	return true;
}
// -----------------------------------------------------------------------------
