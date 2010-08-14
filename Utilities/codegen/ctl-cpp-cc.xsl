<?xml version='1.0' encoding="koi8-r" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version='1.0'
		             xmlns:date="http://exslt.org/dates-and-times">

<xsl:import href="ctl-cpp-common.xsl"/>
<xsl:output method="text" indent="yes" encoding="koi8-r"/>

<xsl:variable name="CLASSNAME">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'class-name'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="BASECLASS">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'base-class'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="OID">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'ID'"/></xsl:call-template>
</xsl:variable>

<!-- Генерирование cc-файла -->
<xsl:template match="/">

<!-- BEGIN CC-FILE -->
// --------------------------------------------------------------------------
<xsl:call-template name="COMMON-CC-HEAD"/>
// --------------------------------------------------------------------------
<xsl:call-template name="COMMON-CC-FILE"/>
// --------------------------------------------------------------------------
<xsl:call-template name="COMMON-CC-FUNCS"/>
// --------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::callback()
{
	if( !active )
		return;
	try
	{	
		// NOTE: Нужно ли здесь опрашивать текущее значение выходов?!!
		// Опрос входов
		updateValues();

		if( idLocalTestMode_S != DefaultObjectId )
		{
			isTestMode = checkTestMode();
			if( trTestMode.change(isTestMode) )
				testMode(isTestMode);

			if( isTestMode )
			{
				if( !active )
					return;

				msleep( sleep_msec );
				return;
			}
		}

		checkSensors();

		// проверка таймеров
		checkTimers(this);

		if( resetMsgTime&gt;0 &amp;&amp; trResetMsg.hi(ptResetMsg.checkTime()) )
		{
//			cout &lt;&lt; myname &lt;&lt;  ": ********* reset messages *********" &lt;&lt; endl;
			resetMsg();
		}

		// обработка сообщений (таймеров и т.п.)
		for( int i=0; i&lt;<xsl:call-template name="settings"><xsl:with-param name="varname" select="'msg-count'"/></xsl:call-template>; i++ )
		{
			if( !receiveMessage(msg) )
				break;
			processingMessage(&amp;msg);
		}

		// Выполнение шага программы
		step();

		// "сердцебиение"
		if( idHeartBeat!=DefaultObjectId &amp;&amp; ptHeartBeat.checkTime() )
		{
			ui.saveValue(idHeartBeat,maxHeartBeat,UniversalIO::AnalogInput);
			ptHeartBeat.reset();
		}

		// Формирование выходов
		updateOutputs(false);
		
		updatePreviousValues();
	}
	catch( Exception&amp; ex )
	{
		unideb[Debug::CRIT] &lt;&lt; myname &lt;&lt; "(execute): " &lt;&lt; ex &lt;&lt; endl;
	}
	catch(CORBA::SystemException&amp; ex)
	{
		unideb[Debug::CRIT] &lt;&lt; myname &lt;&lt; "(execute): СORBA::SystemException: "
			&lt;&lt; ex.NP_minorString() &lt;&lt; endl;
	}
	catch(...)
	{
		unideb[Debug::CRIT] &lt;&lt; myname &lt;&lt; "(execute): catch ..." &lt;&lt; endl;
	}

	if( !active )
		return;
	
	msleep( sleep_msec );
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::setValue( UniSetTypes::ObjectId sid, long val )
{
//	ui.setState(sid,state);
	<xsl:for-each select="//smap/item">
		<xsl:if test="normalize-space(@vartype)='out'">
		if( sid == <xsl:value-of select="@name"/> )
		{
			unideb[Debug::LEVEL2] &lt;&lt;  "(setValue): <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = " &lt;&lt;  val &lt;&lt;  endl;
			<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>	= val;
			return;
		}
		</xsl:if>
		<xsl:if test="normalize-space(@vartype)='io'">
		if( sid == <xsl:value-of select="@name"/> )
		{
			unideb[Debug::LEVEL2] &lt;&lt;  "(setValue): <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = " &lt;&lt;  val &lt;&lt;  endl;
			<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>	= val;
			return;
		}
		</xsl:if>
	</xsl:for-each>
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::updateOutputs( bool force )
{
<xsl:for-each select="//smap/item">
<xsl:choose>
	<xsl:when test="normalize-space(@vartype)='out'"><xsl:call-template name="setdata"/></xsl:when>
	<xsl:when test="normalize-space(@vartype)='io'"><xsl:call-template name="setdata"/></xsl:when>
</xsl:choose>
</xsl:for-each>

<!--
// update messages
<xsl:for-each select="//msgmap/item">
	si.id = <xsl:value-of select="@name"/>;
	ui.saveState( si,m_<xsl:value-of select="@name"/>,UniversalIO::DigitalInput,getId() );
</xsl:for-each>
-->
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::askSensors( UniversalIO::UIOCommand cmd )
{
	<xsl:for-each select="//smap/item">
		<xsl:choose>
			<xsl:when test="normalize-space(@vartype)='in'"><xsl:call-template name="check_changes"><xsl:with-param name="onlymsg" select="1"/></xsl:call-template></xsl:when>
			<xsl:when test="normalize-space(@vartype)='io'"><xsl:call-template name="check_changes"><xsl:with-param name="onlymsg" select="1"/></xsl:call-template></xsl:when>
		</xsl:choose>			
	</xsl:for-each>
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::askState( UniSetTypes::ObjectId sid, UniversalIO::UIOCommand cmd, UniSetTypes::ObjectId node )
{
// #warning НЕ РЕАЛИЗОВАНА...
	if( cmd == UniversalIO::UIONotify )
	{
		SensorMessage sm( sid, (bool)ui.getState(sid,node) );
		sm.node = node;
//		push( sm.transport_msg() );
		sensorInfo(&amp;sm);
	}
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::askValue( UniSetTypes::ObjectId sid, UniversalIO::UIOCommand cmd, UniSetTypes::ObjectId node )
{
// #warning НЕ РЕАЛИЗОВАНА..
	if( cmd == UniversalIO::UIONotify )
	{
		SensorMessage sm( sid, (long)ui.getValue(sid,node) );
		sm.node = node;
//		push( sm.transport_msg() );
		sensorInfo(&amp;sm);
	}
}
// -----------------------------------------------------------------------------
bool <xsl:value-of select="$CLASSNAME"/>_SK::getState( UniSetTypes::ObjectId sid )
{
	<xsl:for-each select="//smap/item">
	if( sid == <xsl:value-of select="@name"/> )
		return <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>;
	</xsl:for-each>

	unideb[Debug::CRIT] &lt;&lt; myname &lt;&lt; "(getState): Обращение к неизвестному ДИСКРЕТНОМУ датчику sid="
		&lt;&lt; sid &lt;&lt; endl;
	return false;
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::preSensorInfo( UniSetTypes::SensorMessage* sm )
{
	sensorInfo(sm);
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::setMsg( UniSetTypes::ObjectId code, bool state )
{
	// блокируем сброс (т.к. он автоматически по таймеру)
	if( !state )
	{
		ptResetMsg.reset();
		return; 
	}

	alarm( code, state );
	ptResetMsg.reset();
}	
// ----------------------------------------------------------------------------
<!-- END CC-FILE -->
</xsl:template>

<xsl:template name="getdata">
<xsl:param name="output">0</xsl:param>	
	if( <xsl:value-of select="@name"/> != DefaultObjectId )
	{
<xsl:choose>
	<xsl:when test="normalize-space(@iotype)='DI'">
		<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = ui.getState(<xsl:value-of select="@name"/>);
	</xsl:when>
	<xsl:when test="normalize-space(@iotype)='AI'">
		<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = ui.getValue(<xsl:value-of select="@name"/>);
	</xsl:when>
</xsl:choose>
<xsl:choose>
	<xsl:when test="normalize-space(@iotype)='DO'">
		<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = ui.getState(<xsl:value-of select="@name"/>);
	</xsl:when>
	<xsl:when test="normalize-space(@iotype)='AO'">
		<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = ui.getValue(<xsl:value-of select="@name"/>);
	</xsl:when>
</xsl:choose>
	}
</xsl:template>

<xsl:template name="setdata">
	if( <xsl:value-of select="@name"/> != DefaultObjectId )
	{
<xsl:choose>
	<xsl:when test="normalize-space(@iotype)='DI'">
		si.id 	= <xsl:value-of select="@name"/>;
		si.node = node_<xsl:value-of select="@name"/>;
		ui.saveState( si, <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>,UniversalIO::DigitalInput,getId() );
	</xsl:when>
	<xsl:when test="normalize-space(@iotype)='DO'">
		ui.setState( <xsl:value-of select="@name"/>, <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>, node_<xsl:value-of select="@name"/> );
	</xsl:when>
	<xsl:when test="normalize-space(@iotype)='AI'">
		si.id 	= <xsl:value-of select="@name"/>;
		si.node = node_<xsl:value-of select="@name"/>;
		ui.saveValue( si, <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>,UniversalIO::AnalogInput, getId() );
	</xsl:when>
	<xsl:when test="normalize-space(@iotype)='AO'">
		ui.setValue( <xsl:value-of select="@name"/>, <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>, node_<xsl:value-of select="@name"/> );
	</xsl:when>
</xsl:choose>
	}
</xsl:template>

<xsl:template name="setdata_value">
<xsl:param name="setval">0</xsl:param>	
	if( <xsl:value-of select="@name"/> != DefaultObjectId )
	{
<xsl:choose>
	<xsl:when test="normalize-space(@iotype)='DI'">
		si.id 	= <xsl:value-of select="@name"/>;
		si.node	= node_<xsl:value-of select="@name"/>;
		ui.saveState( si,<xsl:value-of select="$setval"/>,UniversalIO::DigitalInput,getId() );
	</xsl:when>
	<xsl:when test="normalize-space(@iotype)='DO'">
		ui.setState( <xsl:value-of select="@name"/>,<xsl:value-of select="$setval"/>, node_<xsl:value-of select="@name"/>);
	</xsl:when>
	<xsl:when test="normalize-space(@iotype)='AI'">
		si.id 	= <xsl:value-of select="@name"/>;
		si.node = node_<xsl:value-of select="@name"/>;
		ui.saveValue( si,<xsl:value-of select="$setval"/>,UniversalIO::AnalogInput, getId() );
	</xsl:when>
	<xsl:when test="normalize-space(@iotype)='AO'">
		ui.setValue( <xsl:value-of select="@name"/>,<xsl:value-of select="$setval"/>,node_<xsl:value-of select="@name"/> );
	</xsl:when>
</xsl:choose>
	}
</xsl:template>

<xsl:template name="setmsg_val">
	if( <xsl:value-of select="@name"/> != DefaultObjectId )
	{
<xsl:choose>
	<xsl:when test="normalize-space(@iotype)='DI'">
		si.id = <xsl:value-of select="@name"/>;
		si.node = node_<xsl:value-of select="@name"/>;
		ui.saveState( si,m_<xsl:value-of select="@name"/>,UniversalIO::DigitalInput,getId() );
	</xsl:when>
	<xsl:when test="normalize-space(@iotype)='DO'">
		ui.setState( <xsl:value-of select="@name"/>,m_<xsl:value-of select="@name"/>,node_<xsl:value-of select="@name"/>);
	</xsl:when>
	<xsl:when test="normalize-space(@iotype)='AI'">
		si.id 	= <xsl:value-of select="@name"/>;
		si.node = node_<xsl:value-of select="@name"/>;
		ui.saveValue( si,(long)m_<xsl:value-of select="@name"/>,UniversalIO::AnalogInput, getId() );
	</xsl:when>
	<xsl:when test="normalize-space(@iotype)='AO'">
		ui.setValue( <xsl:value-of select="@name"/>, (long)m_<xsl:value-of select="@name"/>,node_<xsl:value-of select="@name"/> );
	</xsl:when>
</xsl:choose>
	}
</xsl:template>


<xsl:template name="check_changes">
<xsl:param name="onlymsg"></xsl:param>	
<xsl:choose>
	<xsl:when test="normalize-space(@iotype)='DI'">
		<xsl:if test="normalize-space($onlymsg)=''">
		if( prev_<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> != <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> )
		</xsl:if>
		{
			if( <xsl:value-of select="@name"/> != DefaultObjectId )
			{
			<xsl:if test="normalize-space($onlymsg)=''">		
//				cout &lt;&lt; myname &lt;&lt; ": (DI) change state <xsl:value-of select="@name"/> set " 
//					&lt;&lt; <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> &lt;&lt; endl;
			</xsl:if>
				SensorMessage sm( <xsl:value-of select="@name"/>, (bool)<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>);
//				push( ui.transport_msg() );
				sensorInfo(&amp;sm);
			}
		}
	</xsl:when>
	<xsl:when test="normalize-space(@iotype)='AI'">
		<xsl:if test="normalize-space($onlymsg)=''">
		if( prev_<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> != <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> )
		</xsl:if>
		{
		<xsl:if test="normalize-space($onlymsg)=''">
//			cout &lt;&lt; myname &lt;&lt; ": (AI) change value <xsl:value-of select="@name"/> set " 
//					&lt;&lt; <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> &lt;&lt; endl;
		</xsl:if>
			SensorMessage sm( <xsl:value-of select="@name"/>, (long)<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>);
//			push( ui.transport_msg() );
			sensorInfo(&amp;sm);
		}
	</xsl:when>
	<xsl:when test="normalize-space(@iotype)='DO'">
		<xsl:if test="normalize-space($onlymsg)=''">
		if( prev_<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> != <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> )
		</xsl:if>
		{
			if( <xsl:value-of select="@name"/> != DefaultObjectId )
			{
			<xsl:if test="normalize-space($onlymsg)=''">
//				cout &lt;&lt; myname &lt;&lt; ": (DO) change state <xsl:value-of select="@name"/> set " 
//						&lt;&lt; <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> &lt;&lt; endl;
			</xsl:if>
				SensorMessage sm( <xsl:value-of select="@name"/>, (bool)<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>);
//				push( ui.transport_msg() );
				sensorInfo(&amp;sm);
			}
		}
	</xsl:when>
	<xsl:when test="normalize-space(@iotype)='AO'">
		<xsl:if test="normalize-space($onlymsg)=''">
		if( prev_<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> != <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> )
		</xsl:if>
		{
			if( <xsl:value-of select="@name"/> != DefaultObjectId )
			{
			<xsl:if test="normalize-space($onlymsg)=''">
//				cout &lt;&lt; myname &lt;&lt; ": (AO) change value <xsl:value-of select="@name"/> set " 
//						&lt;&lt; <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> &lt;&lt; endl;
			</xsl:if>
				SensorMessage sm( <xsl:value-of select="@name"/>, (long)<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>);
//				push( ui.transport_msg() );
				sensorInfo(&amp;sm);
			}
		}
	</xsl:when>
</xsl:choose>
</xsl:template>

</xsl:stylesheet>
