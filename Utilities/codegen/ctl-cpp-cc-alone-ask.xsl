<?xml version='1.0' encoding="utf-8" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version='1.0'
		             xmlns:date="http://exslt.org/dates-and-times">

<xsl:import href="ctl-cpp-common.xsl"/>
<xsl:output method="text" indent="yes" encoding="utf-8"/>
<!-- Генерирование cc-файла -->

<xsl:variable name="CLASSNAME">
	<xsl:call-template name="settings-alone"><xsl:with-param name="varname" select="'class-name'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="BASECLASS">
	<xsl:call-template name="settings-alone"><xsl:with-param name="varname" select="'base-class'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="OID">
	<xsl:call-template name="settings-alone"><xsl:with-param name="varname" select="'ID'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="TESTMODE">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'testmode'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="ARGPREFIX">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'arg-prefix'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="LOGROTATE">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'logrotate'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="SIMPLEPROC">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'simple-proc'"/></xsl:call-template>
</xsl:variable>

<xsl:template match="/">
<!-- BEGIN CC-FILE -->
// Метод с использованием заказа датчиков
// --------------------------------------------------------------------------
<xsl:call-template name="COMMON-CC-HEAD"/>
// --------------------------------------------------------------------------
<xsl:call-template name="COMMON-CC-ALONE-FUNCS"/>
// --------------------------------------------------------------------------
<xsl:call-template name="COMMON-CC-FILE"/>
// --------------------------------------------------------------------------
<xsl:if test="normalize-space($SIMPLEPROC)!=''">
void <xsl:value-of select="$CLASSNAME"/>_SK::callback() noexcept
{
	if( !active )
		return;
	UniSetObject::callback();
}
</xsl:if>
<xsl:if test="normalize-space($SIMPLEPROC)=''">
void <xsl:value-of select="$CLASSNAME"/>_SK::callback() noexcept
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
		for( unsigned int i=0; i&lt;<xsl:call-template name="settings-alone"><xsl:with-param name="varname" select="'msg-count'"/></xsl:call-template>; i++ )
		{
			auto m = receiveMessage();
			if( !m )
				break;
			processingMessage(m.get());

			// обновление выходов
			updateOutputs(forceOut);
//			updatePreviousValues();
		}

		// Выполнение шага программы
		step();

		// "сердцебиение"
		if( idHeartBeat!=DefaultObjectId &amp;&amp; ptHeartBeat.checkTime() )
		{
			try
			{
				ui->setValue(idHeartBeat,maxHeartBeat);
				ptHeartBeat.reset();
			}
			catch( const uniset::Exception&amp; ex )
			{
				mycrit &lt;&lt; myname &lt;&lt; "(execute): " &lt;&lt; ex &lt;&lt; endl;
			}
		}

		// обновление выходов
		updateOutputs(forceOut);
		updatePreviousValues();
	}
	catch( const uniset::Exception&amp; ex )
	{
        mycrit &lt;&lt; myname &lt;&lt; "(execute): " &lt;&lt; ex &lt;&lt; endl;
	}
	catch( const CORBA::SystemException&amp; ex )
	{
        mycrit &lt;&lt; myname &lt;&lt; "(execute): СORBA::SystemException: "
			&lt;&lt; ex.NP_minorString() &lt;&lt; endl;
	}
    catch( const std::exception&amp;ex )
    {
        mycrit &lt;&lt; myname &lt;&lt; "(execute): catch " &lt;&lt; ex.what()  &lt;&lt;   endl;
    }

	if( !active )
		return;
	
	msleep( sleep_msec );
}
</xsl:if>
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::preSensorInfo( const uniset::SensorMessage* _sm )
{
	<xsl:for-each select="//sensors/item/consumers/consumer">
	<xsl:if test="normalize-space(../../@msg)!='1'">
	<xsl:if test="normalize-space(@name)=$OID">	
	<xsl:if test="normalize-space(@vartype)='in'">
    <xsl:if test="normalize-space(@loglevel)=''">
	if( _sm->id == <xsl:value-of select="../../@name"/> )
		priv_<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/> = _sm->value;
    </xsl:if>
    <xsl:if test="normalize-space(@loglevel)!=''">
	if( _sm->id == <xsl:value-of select="../../@name"/> )
	{
		priv_<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/> = _sm->value;
        mylog->level( Debug::type(_sm->value) );
    }
    </xsl:if>
	</xsl:if>
	</xsl:if>
	</xsl:if>
	</xsl:for-each>

	sensorInfo(_sm);
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::initFromSM()
{
<xsl:for-each select="//sensors/item/consumers/consumer">
	<xsl:if test="normalize-space(../../@msg)!='1'">
	<xsl:if test="normalize-space(@name)=$OID">	
	<xsl:if test="normalize-space(@initFromSM)!=''">
	if( <xsl:value-of select="../../@name"/> != uniset::DefaultObjectId )
	{
		try
		{
			<xsl:if test="normalize-space(@vartype)='in'">priv_</xsl:if><xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/> = ui->getValue(<xsl:value-of select="../../@name"/>,node_<xsl:value-of select="../../@name"/>);
		}
		catch( std::exception&amp; ex )
		{
			mycrit &lt;&lt; myname &lt;&lt; "(initFromSM): " &lt;&lt; ex.what() &lt;&lt; endl;
		}
	}
	</xsl:if>
	</xsl:if>
	</xsl:if>
</xsl:for-each>
}
// -----------------------------------------------------------------------------
uniset::ObjectId <xsl:value-of select="$CLASSNAME"/>_SK::getSMTestID() const
{
	if( smTestID != DefaultObjectId )
		return smTestID;

	<xsl:for-each select="//sensors/item/consumers/consumer">
	<xsl:if test="normalize-space(@name)=$OID">
	if( <xsl:value-of select="../../@name"/> != DefaultObjectId )
		return <xsl:value-of select="../../@name"/>;
	</xsl:if>
	</xsl:for-each>

	return DefaultObjectId;
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
		mycrit &lt;&lt; myname
			&lt;&lt; "(preAskSensors): ************* don`t activated?! ************" &lt;&lt; endl;

	while( !cancelled )
	{
		try
		{
		<xsl:for-each select="//sensors/item/consumers/consumer">
		<xsl:if test="normalize-space(@name)=$OID">
		<xsl:if test="normalize-space(@vartype)='in'">
			ui->askRemoteSensor(<xsl:value-of select="../../@name"/>,_cmd,node_<xsl:value-of select="../../@name"/>, getId());
		</xsl:if>
		</xsl:if>
		</xsl:for-each>
			return;
		}
		catch( const uniset::Exception&amp; ex )
		{
            mycrit &lt;&lt; myname &lt;&lt; "(preAskSensors): " &lt;&lt; ex &lt;&lt; endl;
		}
    	catch( const std::exception&amp;ex )
	    {
	    mycrit &lt;&lt; myname &lt;&lt; "(execute): catch " &lt;&lt; ex.what()  &lt;&lt;   endl;
	    }
    	msleep(askPause);
	}
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::setValue( uniset::ObjectId _sid, long _val )
{
    if( _sid == uniset::DefaultObjectId )
        return;
        
//	ui->setState(sid,state);
	<xsl:for-each select="//sensors/item/consumers/consumer">
	<xsl:if test="normalize-space(@name)=$OID">
	<xsl:if test="normalize-space(../../@msg)!='1'">
	<xsl:if test="normalize-space(@vartype)='out'">
	if( _sid == <xsl:value-of select="../../@name"/> )
	{
        mylog8 &lt;&lt;  "(setValue): <xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/> = " &lt;&lt; _val &lt;&lt;  endl;
		<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>	= _val;
		<xsl:call-template name="setdata"/>
		return;
	}
	</xsl:if>
	</xsl:if>
	</xsl:if>
	</xsl:for-each>

	ui->setValue(_sid,_val);
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::askSensor( uniset::ObjectId _sid, UniversalIO::UIOCommand _cmd, uniset::ObjectId _node )
{
	ui->askRemoteSensor(_sid,_cmd,_node,getId());
}

// -----------------------------------------------------------------------------
long <xsl:value-of select="$CLASSNAME"/>_SK::getValue( uniset::ObjectId _sid )
{
	try
	{
	<xsl:for-each select="//sensors/item/consumers/consumer">
	<xsl:if test="normalize-space(@name)=$OID">
	<xsl:if test="normalize-space(../../@msg)!='1'">
		if( _sid == <xsl:value-of select="../../@name"/> )
		{
		<xsl:if test="normalize-space(@vartype)='in'">/*		<xsl:text>		</xsl:text>priv_<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/> = ui->getValue(<xsl:value-of select="../../@name"/>); */</xsl:if>
			return <xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>;
		}
	</xsl:if>
	</xsl:if>
	</xsl:for-each>

		return ui->getValue(_sid);
	}
	catch( const uniset::Exception&amp; ex )
	{
		mycrit &lt;&lt; myname &lt;&lt; "(getValue): " &lt;&lt; ex &lt;&lt; endl;
		throw;
	}
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::updateOutputs( bool _force )
{
<xsl:for-each select="//sensors/item/consumers/consumer">
	<xsl:if test="normalize-space(@name)=$OID">
	<xsl:if test="normalize-space(../../@msg)!='1'">
	<xsl:choose>
	<xsl:when test="normalize-space(@vartype)='out'">
	<xsl:if test="normalize-space(../../@force)=''">
	if( _force || prev_<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/> != <xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/> )
	</xsl:if>
	{
		<xsl:call-template name="setdata"/>
		prev_<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/> = <xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>;
	}
	</xsl:when>
	</xsl:choose>
	</xsl:if>
	</xsl:if>
</xsl:for-each>

<xsl:for-each select="//sensors/item/consumers/consumer">
<xsl:if test="normalize-space(@name)=$OID">
<xsl:if test="normalize-space(../../@msg)='1'">
	if( _force || prev_m_<xsl:value-of select="../../@name"/> != m_<xsl:value-of select="../../@name"/> )
	{
		si.id 	= mid_<xsl:value-of select="../../@name"/>;
		ui->setValue( si,m_<xsl:value-of select="../../@name"/>, getId() );
		prev_m_<xsl:value-of select="../../@name"/> = m_<xsl:value-of select="../../@name"/>;
	}
</xsl:if>
</xsl:if>
</xsl:for-each>
}
// -----------------------------------------------------------------------------
<!-- END CC-FILE -->
</xsl:template>

<xsl:template name="getdata">
<xsl:param name="output">0</xsl:param>	
	try
	{
<xsl:if test="normalize-space(../../@msg)!='1'">
<xsl:if test="normalize-space(@name)=$OID">
		priv_<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/> = ui->getValue(<xsl:value-of select="../../@name"/>);
</xsl:if>
</xsl:if>
	}
	catch( const uniset::Exception&amp; ex )
	{
		mycrit &lt;&lt; myname &lt;&lt; "(getdata): " &lt;&lt; ex &lt;&lt; endl;
		throw;
	}
</xsl:template>

<xsl:template name="setdata">
<xsl:if test="normalize-space(../../@msg)!='1'">
<xsl:if test="normalize-space(@name)=$OID">
<xsl:choose>
	<xsl:when test="normalize-space(@vartype)='out'"><xsl:call-template name="gensetdata"/></xsl:when>
</xsl:choose>
</xsl:if>
</xsl:if>
</xsl:template>

<xsl:template name="setdata_value">
<xsl:param name="setval"></xsl:param>	
<xsl:if test="normalize-space(../../@msg)!='1'">
<xsl:if test="normalize-space(@name)=$OID">
<xsl:choose>
	<xsl:when test="normalize-space(@vartype)='out'"><xsl:call-template name="setdata_val"><xsl:with-param name="setval" select="0"/></xsl:call-template></xsl:when>
</xsl:choose>
</xsl:if>
</xsl:if>
</xsl:template>

<xsl:template name="gensetdata">
		try
		{
			si.id 	= <xsl:value-of select="../../@name"/>;
			si.node = node_<xsl:value-of select="../../@name"/>;
			ui->setValue( si,<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>, getId() );
		}
		catch( const uniset::Exception&amp; ex )
		{
			mycrit &lt;&lt; myname &lt;&lt; "(setdata): " &lt;&lt; ex &lt;&lt; endl;
			throw;
		}
</xsl:template>

<xsl:template name="setdata_val">
<xsl:param name="setval"></xsl:param>
	try
	{
		si.id 	= <xsl:value-of select="../../@name"/>;
		si.node = node_<xsl:value-of select="../../@name"/>;
		ui->setValue( si,<xsl:value-of select="$setval"/>, getId() );
	}
	catch( const uniset::Exception&amp; ex )
	{
		mycrit &lt;&lt; myname &lt;&lt; "(setdata): " &lt;&lt; ex &lt;&lt; endl;
		throw;
	}
</xsl:template>

<xsl:template name="check_changes">
<xsl:param name="onlymsg"></xsl:param>	
<xsl:if test="normalize-space(../../@msg)!='1'">
<xsl:if test="normalize-space(@name)=$OID">
	<xsl:if test="normalize-space($onlymsg)=''">
	if( prev_<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/> != <xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/> )
	</xsl:if>
	{
		// приходится искуственно использовать третий параметр, 
		// что-бы компилятор выбрал
		// правильный(для аналоговых) конструктор у SensorMessage
		IOController_i::CalibrateInfo _ci;
		SensorMessage _sm( <xsl:value-of select="../../@name"/>, (long)<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>, _ci);
		_sm.sensor_type = UniversalIO::AI;
		sensorInfo(&amp;_sm);
	}
</xsl:if>
</xsl:if>
</xsl:template>

</xsl:stylesheet>
