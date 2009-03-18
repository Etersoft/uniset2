// $Id: IOBase.cc,v 1.3 2009/01/23 23:56:54 vpashka Exp $
// -----------------------------------------------------------------------------
#include "Configuration.h"
#include "Extentions.h"
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
	if( breaklim <=0 )
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
void IOBase::processingAsAI( IOBase* it, long val, SMInterface* shm, bool force )
{
	// проверка на обрыв
	if( it->check_channel_break(val) )
	{
		uniset_spin_lock lock(it->val_lock);
		it->value = ChannelBreakValue;
		shm->localSetUndefinedState(it->ait,true,it->si.id);
		return;
	}

	// Читаем с использованием фильтра...
	if( !it->nofilter )
	{
		if( it->df.size() > 1 )
			it->df.add(val);
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
			val = it->cprev;	// просто передаём предыдущее значение
	}
	else
	{
		IOController_i::CalibrateInfo* cal( &(it->cal) );
		if( cal->maxRaw!=0 && cal->maxRaw!=cal->minRaw ) // задана обычная калибровка
			val = UniSetTypes::lcalibrate(val,cal->minRaw,cal->maxRaw,cal->minCal,cal->maxCal,true);
	}

	// если предыдущее значение "обрыв",
	// то сбрасываем признак 
	{
		uniset_spin_lock lock(it->val_lock);
		if( it->value == ChannelBreakValue )
			shm->localSetUndefinedState(it->ait,false,it->si.id);

		if( it->cal.precision > 0 )
			it->value *= lround(pow10(it->cal.precision));

		if( force || it->value != val )
		{
			if( it->stype == UniversalIO::AnalogInput )
				shm->localSaveValue( it->ait,it->si.id,val,shm->ID() );
			else if( it->stype == UniversalIO::AnalogOutput )
				shm->localSetValue( it->ait,it->si.id,val,shm->ID() );

			it->value = val;
		}
	}
}
// -----------------------------------------------------------------------------
void IOBase::processingAsDI( IOBase* it, bool set, SMInterface* shm, bool force )
{
//	cout  << "subdev: " << it->subdev << " chan: " << it->channel << " state=" << set << endl;
	if( it->invert )
		set ^= true;
//	cout  << "subdev: " << it->subdev << " chan: " << it->channel << " (inv)state=" << set << endl;

	// Проверяем именно в такой последовательности!
	set = it->check_jar(set);		// фильтр дребезга
//	cout  << "subdev: " << it->subdev << " chan: " << it->channel << " (jar)state=" << set << endl;
	set = it->check_on_delay(set);	// фильтр на срабатывание
//	cout  << "subdev: " << it->subdev << " chan: " << it->channel << " (on_delay)state=" << set << endl;
	set = it->check_off_delay(set);	// фильтр на отпускание
//	cout  << "subdev: " << it->subdev << " chan: " << it->channel << " (off_delay)state=" << set << endl;

	{
		uniset_spin_lock lock(it->val_lock);
		if( force || (bool)it->value!=set )
		{
			if( it->stype == UniversalIO::DigitalInput )
				shm->localSaveState(it->dit,it->si.id,set,shm->ID());
			else if( it->stype == UniversalIO::DigitalOutput )
				shm->localSetState(it->dit,it->si.id,set,shm->ID());
			
			it->value = set ? 1 : 0;
		}
	}
}
// -----------------------------------------------------------------------------
long IOBase::processingAsAO( IOBase* it, SMInterface* shm, bool force )
{
	uniset_spin_lock lock(it->val_lock);

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
			IOController_i::CalibrateInfo* cal( &(it->cal) );
			if( cal->maxRaw!=0 && cal->maxRaw!=cal->minRaw ) // задана калибровка
			{
				// Калибруем в обратную сторону!!!
				val = UniSetTypes::lcalibrate(it->value,
							cal->minCal, cal->maxCal, cal->minRaw, cal->maxRaw, true );
			}
			else
				val = it->value;
#warning Precision!!!
//			if( it->cal.precision > 0 )
//				val = it->value / lround(pow10(it->cal.precision));
		}
	}
	
	return val;
}
// -----------------------------------------------------------------------------
bool IOBase::processingAsDO( IOBase* it, SMInterface* shm, bool force )
{
	uniset_spin_lock lock(it->val_lock);
	bool set = it->value;
	if( it->stype == UniversalIO::DigitalOutput ||
		it->stype == UniversalIO::DigitalInput )
	{
		if( force )
			set = shm->localGetState(it->dit,it->si.id);
		
		set = it->invert ? !set : set;
		return set; 
	}
	
	return false;
}
// -----------------------------------------------------------------------------
bool IOBase::initItem( IOBase* b, UniXML_iterator& it, SMInterface* shm,  
						DebugStream* dlog, std::string myname,
						int def_filtersize, float def_filterT )
{
	string sname( it.getProp("name") );

	ObjectId sid = DefaultObjectId;
	if( it.getProp("id").empty() )
		sid = conf->getSensorID(sname);
	else
	{
		sid = UniSetTypes::uni_atoi(it.getProp("id").c_str());
		if( sid <=0 )
			sid = DefaultObjectId;
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

	b->nofilter = atoi( it.getProp("nofilter").c_str() );
	b->ignore	= atoi( it.getProp("ioignore").c_str() );
	b->invert	= atoi( it.getProp("ioinvert").c_str() );
	b->defval 	= atoi( it.getProp("default").c_str() );
	b->value	= b->defval;
	b->breaklim = atoi( it.getProp("breaklim").c_str() );
	
	long msec = atoi( it.getProp("jardelay").c_str() );
	b->ptJar.setTiming(msec);
	if( msec<=0 )
		b->ptJar.setTiming(UniSetTimer::WaitUpTime);

	msec = atoi( it.getProp("ondelay").c_str() );
	b->ptOnDelay.setTiming(msec);
	if( msec<=0 )
		b->ptOnDelay.setTiming(UniSetTimer::WaitUpTime);

	msec = atoi( it.getProp("offdelay").c_str() );
	b->ptOffDelay.setTiming(msec);
	if( msec<=0 )
		b->ptOffDelay.setTiming(UniSetTimer::WaitUpTime);
	
	string saf = it.getProp("safety");
	if( !saf.empty() )
		b->safety = atoi(saf.c_str());
	else
		b->safety = NoSafety;

	string stype( it.getProp("iotype") );
	if( stype == "AI" )
		b->stype = UniversalIO::AnalogInput;
	else if ( stype == "AO" )
		b->stype = UniversalIO::AnalogOutput;
	else if ( stype == "DO" )
		b->stype = UniversalIO::DigitalOutput;
	else if ( stype == "DI" )
		b->stype = UniversalIO::DigitalInput;
	else
	{
		if( dlog )
			dlog[Debug::CRIT] << myname << "(IOBase::readItem): неизвестный iotype=: " 
				<< stype << " для " << sname << endl;
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
		
	shm->initAIterator(b->ait);
	shm->initDIterator(b->dit);

	if( b->stype == UniversalIO::AnalogInput || b->stype == UniversalIO::AnalogOutput )
	{
		b->cal.minRaw = atoi( it.getProp("rmin").c_str() );
		b->cal.maxRaw = atoi( it.getProp("rmax").c_str() );
		b->cal.minCal = atoi( it.getProp("cmin").c_str() );
		b->cal.maxCal = atoi( it.getProp("cmax").c_str() );
		b->cal.sensibility = atoi( it.getProp("sensibility").c_str() );
		b->cal.precision = atoi( it.getProp("precision").c_str() );

		int f_size 	= def_filtersize;
		float f_T 	= def_filterT;
		int f_median = atoi( it.getProp("filtermedian").c_str() );
		
		if( f_median > 0 )
		{
			f_size = f_median;
			b->f_median = true;
		}
		else
		{
			if( !it.getProp("filtersize").empty() )
			{
				f_size = atoi(it.getProp("filtersize").c_str());
				if( f_size < 0 )
					f_size = 0;
			}
		}
		
		for( int i=0; i<f_size; i++ )
			b->df.add( b->defval );

		if( !it.getProp("filterT").empty() )
		{
			f_T = atof(it.getProp("filterT").c_str());
			if( f_T < 0 )
				f_T = 0.0;
		}

		if( b->stype == UniversalIO::AnalogInput )
			b->df.setSettings( f_size, f_T );

		std::string caldiagram( it.getProp("caldiagram") );
		if( !caldiagram.empty() )
			b->cdiagram = UniSetExtentions::buildCalibrationDiagram(caldiagram);
	}

	return true;
}
// -----------------------------------------------------------------------------
