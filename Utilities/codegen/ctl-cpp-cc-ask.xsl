<?xml version='1.0' encoding="utf-8" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version='1.0'
		             xmlns:date="http://exslt.org/dates-and-times">

<xsl:import href="ctl-cpp-common.xsl"/>
<xsl:output method="text" indent="yes" encoding="utf-8"/>

<xsl:variable name="CLASSNAME">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'class-name'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="BASECLASS">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'base-class'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="OID">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'ID'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="TESTMODE">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'testmode'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="ARGPREFIX">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'arg-prefix'"/></xsl:call-template>
</xsl:variable>

<!-- Генерирование cc-файла -->
<xsl:template match="/">

<!-- BEGIN CC-FILE -->
// Метод с использованием заказа датчиков
// --------------------------------------------------------------------------
<xsl:call-template name="COMMON-CC-HEAD"/>
// --------------------------------------------------------------------------
<xsl:call-template name="COMMON-CC-FUNCS"/>
// --------------------------------------------------------------------------
<xsl:call-template name="COMMON-CC-FILE"/>
// --------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::callback()
{
	if( !active )
		return;
	try
	{
<xsl:if test="normalize-space($TESTMODE)!=''">
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
</xsl:if>
		// проверка таймеров
		checkTimers(this);

		if( resetMsgTime&gt;0 &amp;&amp; trResetMsg.hi(ptResetMsg.checkTime()) )
		{
//			cout &lt;&lt; myname &lt;&lt;  ": ********* reset messages *********" &lt;&lt; endl;
			resetMsg();
		}

		// обработка сообщений (таймеров и т.п.)
		for( unsigned int i=0; i&lt;<xsl:call-template name="settings"><xsl:with-param name="varname" select="'msg-count'"/></xsl:call-template>; i++ )
		{
			if( !receiveMessage(msg) )
				break;
			processingMessage(&amp;msg);
			updateOutputs(forceOut);
//			updatePreviousValues();
		}

		// Выполнение шага программы
		step();

		// "сердцебиение"
		if( idHeartBeat!=DefaultObjectId &amp;&amp; ptHeartBeat.checkTime() )
		{
			ui->setValue(idHeartBeat,maxHeartBeat,UniversalIO::AI);
			ptHeartBeat.reset();
		}

		// обновление выходов
		updateOutputs(forceOut);
		updatePreviousValues();
	}
	catch( Exception&amp; ex )
	{
        ucrit &lt;&lt; myname &lt;&lt; "(execute): " &lt;&lt; ex &lt;&lt; endl;
	}
	catch(CORBA::SystemException&amp; ex)
	{
        ucrit &lt;&lt; myname &lt;&lt; "(execute): СORBA::SystemException: "
                &lt;&lt; ex.NP_minorString() &lt;&lt; endl;
	}
    catch( std::exception&amp;ex )
    {
        ucrit &lt;&lt; myname &lt;&lt; "(execute): catch " &lt;&lt; ex.what()  &lt;&lt;   endl;
    }

	if( !active )
		return;
	
	msleep( sleep_msec );
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::setValue( UniSetTypes::ObjectId _sid, long _val )
{
	<xsl:for-each select="//smap/item">
		<xsl:if test="normalize-space(@vartype)='out'">
		if( _sid == <xsl:value-of select="@name"/> )
		{
            ulog2 &lt;&lt;  "(setState): <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = " &lt;&lt;  _val &lt;&lt;  endl;
			<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>	= _val;
				<xsl:call-template name="setdata"/>
			return;
		}
		</xsl:if>
	</xsl:for-each>

	ui->setValue(_sid,_val);
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::updateOutputs( bool _force )
{
	<xsl:for-each select="//smap/item">
	<xsl:if test="normalize-space(@force)=''">
	if( _force || prev_<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> != <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> )
	</xsl:if>
	{
		<xsl:choose>
		<xsl:when test="normalize-space(@vartype)='out'"><xsl:call-template name="setdata"/></xsl:when>
		</xsl:choose>
		prev_<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>;
	}
	</xsl:for-each>

<!--
// update messages
<xsl:for-each select="//msgmap/item">
	if( prev_m_<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> != m_<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> )
	{
		si.id = <xsl:value-of select="@name"/>;
		ui->setValue( si,m_<xsl:value-of select="@name"/>,getId() );
		prev_m_<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = m_<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>;		
	}
</xsl:for-each>
-->
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::preSensorInfo( const UniSetTypes::SensorMessage* _sm )
{
	<xsl:for-each select="//smap/item">
	<xsl:if test="normalize-space(@vartype)='in'">
	<xsl:if test="normalize-space(@loglevel)=''">
	if( _sm->id == <xsl:value-of select="@name"/> )
			<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = _sm->value;
	</xsl:if>
	<xsl:if test="normalize-space(@loglevel)!=''">
	if( _sm->id == <xsl:value-of select="@name"/> )
	{
		<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = _sm->value;
		mylog.level( Debug::type(_sm->value) );
	}
	</xsl:if>
	</xsl:if>
	</xsl:for-each>
	
	sensorInfo(_sm);
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::askSensor( UniSetTypes::ObjectId _sid, UniversalIO::UIOCommand _cmd, UniSetTypes::ObjectId _node )
{
	ui->askRemoteSensor(_sid,_cmd,_node,getId());
}
// -----------------------------------------------------------------------------
long <xsl:value-of select="$CLASSNAME"/>_SK::getValue( UniSetTypes::ObjectId _sid )
{
	try
	{
<xsl:for-each select="//smap/item">
		if( _sid == <xsl:value-of select="@name"/> &amp;&amp; <xsl:value-of select="@name"/> != DefaultObjectId )
		{
		<xsl:text>		</xsl:text><xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = ui->getValue(<xsl:value-of select="@name"/>, node_<xsl:value-of select="@name"/>);
			return <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>;
		}
</xsl:for-each>

		return ui->getValue(_sid);
	}
	catch(Exception&amp; ex)
	{
        ucrit &lt;&lt; myname &lt;&lt; "(getValue): " &lt;&lt; ex &lt;&lt; endl;
		throw;
	}
}
// -----------------------------------------------------------------------------

void <xsl:value-of select="$CLASSNAME"/>_SK::preAskSensors( UniversalIO::UIOCommand _cmd )
{
	PassiveTimer ptAct(activateTimeout);
	while( !activated &amp;&amp; !ptAct.checkTime() )
	{	
		cout &lt;&lt; myname &lt;&lt; "(preAskSensors): wait activate..." &lt;&lt; endl;
		msleep(300);
		if( activated )
			break;
	}
			
	if( !activated )
		ucrit &lt;&lt; myname
			&lt;&lt; "(preAskSensors): ************* don`t activated?! ************" &lt;&lt; endl;

	for( ;; )
	{
		try
		{
		<xsl:for-each select="//smap/item">
		<xsl:if test="normalize-space(@vartype)='in'">
			if( <xsl:value-of select="@name"/> != DefaultObjectId )
				ui->askRemoteSensor(<xsl:value-of select="@name"/>,_cmd,node_<xsl:value-of select="@name"/>,getId());
		</xsl:if> 
		</xsl:for-each>
			return;
		}
		catch(SystemError&amp; err)
		{
            ucrit &lt;&lt; myname &lt;&lt; "(preAskSensors): " &lt;&lt; err &lt;&lt; endl;
		}
		catch(Exception&amp; ex)
		{
            ucrit &lt;&lt; myname &lt;&lt; "(preAskSensors): " &lt;&lt; ex &lt;&lt; endl;
		}
	    catch( std::exception&amp;ex )
    	{
        	ucrit &lt;&lt; myname &lt;&lt; "(execute): catch " &lt;&lt; ex.what()  &lt;&lt;   endl;
	    }

		msleep(askPause);
	}
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::setMsg( UniSetTypes::ObjectId _code, bool _state )
{
	// блокируем сброс (т.к. он автоматически по таймеру)
	if( !_state )
	{
		ptResetMsg.reset();
		return; 
	}

	alarm( _code, _state );
	ptResetMsg.reset();
}	
// ----------------------------------------------------------------------------
<!-- END CC-FILE -->
</xsl:template>

<xsl:template name="getdata">
<xsl:param name="output">0</xsl:param>	
	if( <xsl:value-of select="@name"/> != DefaultObjectId )
	{
		try
		{
			<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = ui->getValue(<xsl:value-of select="@name"/>, node_<xsl:value-of select="@name"/>);
		}
		catch( UniSetTypes::Exception&amp; ex )
		{
            ucrit &lt;&lt; myname &lt;&lt; "(getdata): " &lt;&lt; ex &lt;&lt; endl;
			throw;
		}
	}
</xsl:template>

<xsl:template name="setdata">
			if( <xsl:value-of select="@name"/> != DefaultObjectId )
			{
				try
				{
					si.id 	= <xsl:value-of select="@name"/>;
					si.node = node_<xsl:value-of select="@name"/>;
					ui->setValue( si, <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>, getId() );
				}
				catch( UniSetTypes::Exception&amp; ex )
				{
                    ucrit &lt;&lt; myname &lt;&lt; "(setdata): " &lt;&lt; ex &lt;&lt; endl;
					throw;
				}
			}
</xsl:template>

<xsl:template name="setdata_value">
<xsl:param name="setval">0</xsl:param>	
	if( <xsl:value-of select="@name"/> != DefaultObjectId )
	{
		try
		{
			si.id 	= <xsl:value-of select="@name"/>;
			si.node = node_<xsl:value-of select="@name"/>;
			ui->setValue( si,<xsl:value-of select="$setval"/>, getId() );
		}
		catch( UniSetTypes::Exception&amp; ex )
		{
            ucrit &lt;&lt; myname &lt;&lt; "(setdata): " &lt;&lt; ex &lt;&lt; endl;
			throw;
		}
	}
</xsl:template>

</xsl:stylesheet>
