<?xml version='1.0' encoding="utf-8" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version='1.0'
		             xmlns:date="http://exslt.org/dates-and-times">

<xsl:output method="text" indent="yes" encoding="utf-8"/>

<xsl:template name="settype">
	<xsl:param name="iotype"/>
<xsl:choose>
	<xsl:when test="$iotype='DO'">bool</xsl:when>
	<xsl:when test="$iotype='DI'">bool</xsl:when>
	<xsl:when test="$iotype='AO'">long</xsl:when>
	<xsl:when test="$iotype='AI'">long</xsl:when>
</xsl:choose>
</xsl:template>

<xsl:template name="setprefix">
<xsl:choose>
	<xsl:when test="normalize-space(@vartype)='in'">in_</xsl:when>
	<xsl:when test="normalize-space(@vartype)='out'">out_</xsl:when>
	<xsl:when test="normalize-space(@vartype)='io'">io_</xsl:when>
	<xsl:when test="normalize-space(@vartype)='none'">nn_</xsl:when>
</xsl:choose>
</xsl:template>

<xsl:template name="preinclude">
	<xsl:if test="normalize-space($LOCALINC)!=''">
	<xsl:text>&quot;</xsl:text>
	</xsl:if>
	<xsl:if test="normalize-space($LOCALINC)=''">
	<xsl:text>&lt;uniset/</xsl:text>
	</xsl:if>
</xsl:template>
<xsl:template name="postinclude">
	<xsl:if test="normalize-space($LOCALINC)!=''">
	<xsl:text>&quot;</xsl:text>
	</xsl:if>
	<xsl:if test="normalize-space($LOCALINC)=''">
	<xsl:text>&gt;</xsl:text>
	</xsl:if>
</xsl:template>

<xsl:template name="setvar">
	<xsl:param name="GENTYPE"/>
	<xsl:if test="normalize-space(@msg)!='1'">
	<xsl:for-each select="./consumers/consumer">
		<xsl:if test="normalize-space(@name)=$OID">
		<xsl:choose>
		<xsl:when test="$GENTYPE='H'">
		const UniSetTypes::ObjectId <xsl:value-of select="../../@name"/>; 		/*!&lt; <xsl:value-of select="../../@textname"/> */
		UniSetTypes::ObjectId node_<xsl:value-of select="../../@name"/>;
		<xsl:call-template name="settype"><xsl:with-param name="iotype" select="../../@iotype" /></xsl:call-template><xsl:text> </xsl:text><xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>; /*!&lt; текущее значение */
		<xsl:call-template name="settype"><xsl:with-param name="iotype" select="../../@iotype" /></xsl:call-template><xsl:text> prev_</xsl:text><xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>; /*!&lt; предыдущее значение */
		</xsl:when>
		<xsl:when test="$GENTYPE='C'"><xsl:value-of select="../../@name"/>(<xsl:value-of select="../../@id"/>),
			<xsl:if test="not(normalize-space(../../@node)='')">
				node_<xsl:value-of select="../../@name"/>(conf->getNodeID("<xsl:value-of select="../../@node"/>")),
			</xsl:if>
			<xsl:if test="normalize-space(../../@node)=''">
				node_<xsl:value-of select="../../@name"/>(UniSetTypes::conf->getLocalNode()),
			</xsl:if>
			<xsl:if test="normalize-space(../../@default)=''">
				<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>(0),
			</xsl:if>
			<xsl:if test="not(normalize-space(../../@default)='')">
				<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>(<xsl:value-of select="../../@default"/>),
			</xsl:if>
		</xsl:when>
		<xsl:when test="$GENTYPE='CHECK'">
			<xsl:if test="normalize-space(../../@id)=''">throw SystemException("Not Found ID for <xsl:value-of select="../../@name"/>");</xsl:if>
		</xsl:when>
		</xsl:choose>
		</xsl:if>
	</xsl:for-each>
	</xsl:if>	
</xsl:template>

<xsl:template name="setmsg">
	<xsl:param name="GENTYPE"/>
	<xsl:if test="normalize-space(@msg)='1'">
	<xsl:for-each select="./consumers/consumer">
		<xsl:if test="normalize-space(@name)=$OID">
			<xsl:choose>
			<xsl:when test="$GENTYPE='H'">const UniSetTypes::ObjectId mid_<xsl:value-of select="normalize-space(../../@name)"/>; 	/*!&lt; <xsl:value-of select="normalize-space(../../@text)"/> */
			bool m_<xsl:value-of select="normalize-space(../../@name)"/>; 	/*!&lt; текущее состояние (сообщения) */
			bool prev_m_<xsl:value-of select="normalize-space(../../@name)"/>; 	/*!&lt; предыдущее состояние (сообщения) */
			</xsl:when>
			<xsl:when test="$GENTYPE='C'">mid_<xsl:value-of select="normalize-space(../../@name)"/>(<xsl:value-of select="../../@id"/>),
			</xsl:when>
			<xsl:when test="$GENTYPE='CHECK'">
				<xsl:if test="normalize-space(@no_check_id)!='1'">
				<xsl:if test="normalize-space(../../@id)=''">unideb[Debug::WARN] &lt;&lt; myname &lt;&lt; ": Not found (Message)OID for mid_<xsl:value-of select="normalize-space(../../@name)"/>" &lt;&lt; endl;
				</xsl:if>
				</xsl:if>
			</xsl:when>
			<xsl:when test="$GENTYPE='U'">
				si.id 	= mid_<xsl:value-of select="../../@name"/>;
				ui.saveState( si,m_<xsl:value-of select="../../@name"/>, UniversalIO::DigitalInput, getId() );
			</xsl:when>		
			<xsl:when test="$GENTYPE='A'">
			if( _code == mid_<xsl:value-of select="../../@name"/> )
			{				
				unideb(Debug::LEVEL8) &lt;&lt; "<xsl:value-of select="../../@name"/>" &lt;&lt; endl;
				m_<xsl:value-of select="../../@name"/> = _state;
				try
				{
					// сохраняем сразу...
					si.id = mid_<xsl:value-of select="../../@name"/>;
					ui.saveState( si,m_<xsl:value-of select="../../@name"/>,UniversalIO::DigitalInput,getId() );
					return true;
				}
				catch(...){}
				return false;
			}
			</xsl:when>		
			<xsl:when test="$GENTYPE='R'">
				m_<xsl:value-of select="../../@name"/> = 0;
				if( mid_<xsl:value-of select="../../@name"/> != UniSetTypes::DefaultObjectId )
				{
				 	try
					{
						si.id = mid_<xsl:value-of select="../../@name"/>;
						ui.saveState( si,false,UniversalIO::DigitalInput,getId() );
					}
					catch( UniSetTypes::Exception&amp; ex )
					{
						unideb[Debug::LEVEL1] &lt;&lt; getName() &lt;&lt; ex &lt;&lt; endl;
					}
				}
			</xsl:when>
			</xsl:choose>
		</xsl:if>
	</xsl:for-each>
	</xsl:if>
</xsl:template>

<xsl:template name="settings">
	<xsl:param name="varname"/>
	<xsl:for-each select="//settings">
		<xsl:for-each select="./*">
			<xsl:if test="normalize-space(@name)=$varname"><xsl:value-of select="@val"/></xsl:if>
		</xsl:for-each>
	</xsl:for-each>
</xsl:template>

<xsl:template name="settings-alone">
	<xsl:param name="varname"/>
	<xsl:for-each select="//settings/*">
		<xsl:if test="normalize-space(@name)=$CNAME">
			<xsl:for-each select="./*">
				<xsl:if test="normalize-space(@name)=$varname"><xsl:value-of select="normalize-space(@val)"/></xsl:if>
			</xsl:for-each>
		</xsl:if>
	</xsl:for-each>
</xsl:template>

<xsl:template name="COMMON-HEAD-PUBLIC">

		bool alarm( UniSetTypes::ObjectId sid, bool state );
		bool getState( UniSetTypes::ObjectId sid );
		bool getValue( UniSetTypes::ObjectId sid );
		void setValue( UniSetTypes::ObjectId sid, long value );
		void setState( UniSetTypes::ObjectId sid, bool state );
		void askState( UniSetTypes::ObjectId sid, UniversalIO::UIOCommand, UniSetTypes::ObjectId node = UniSetTypes::conf->getLocalNode() );
		void askValue( UniSetTypes::ObjectId sid, UniversalIO::UIOCommand, UniSetTypes::ObjectId node = UniSetTypes::conf->getLocalNode() );
		void updateValues();
		void setMsg( UniSetTypes::ObjectId code, bool state );
</xsl:template>

<xsl:template name="COMMON-HEAD-PROTECTED">
		virtual void callback();
		virtual void processingMessage( UniSetTypes::VoidMessage* msg );
		virtual void sysCommand( UniSetTypes::SystemMessage* sm );
		virtual void askSensors( UniversalIO::UIOCommand cmd );
		virtual void sensorInfo( UniSetTypes::SensorMessage* sm ){};
		virtual void timerInfo( UniSetTypes::TimerMessage* tm ){};
		virtual void sigterm( int signo );
		virtual bool activateObject();
		virtual void testMode( bool state );
		void updatePreviousValues();
		void checkSensors();
		void updateOutputs( bool force );
		bool checkTestMode();
		void preSensorInfo( UniSetTypes::SensorMessage* sm );
		void preTimerInfo( UniSetTypes::TimerMessage* tm );
		void waitSM( int wait_msec, UniSetTypes::ObjectId testID = UniSetTypes::DefaultObjectId );

		void resetMsg();
		Trigger trResetMsg;
		PassiveTimer ptResetMsg;
		int resetMsgTime;

		// Выполнение очередного шага программы
		virtual void step()=0;

		int sleep_msec; /*!&lt; пауза между итерациями */
		bool active;
		bool isTestMode;
		Trigger trTestMode;
		UniSetTypes::ObjectId idTestMode_S;		  	/*!&lt; идентификатор для флага тестовго режима (для всех) */
		UniSetTypes::ObjectId idLocalTestMode_S;	/*!&lt; идентификатор для флага тестовго режима (для данного узла) */
		bool in_TestMode_S;
		bool in_LocalTestMode_S;

		// управление датчиком "сердцебиения"
		PassiveTimer ptHeartBeat;				/*! &lt; период "сердцебиения" */
		UniSetTypes::ObjectId idHeartBeat;		/*! &lt; идентификатор датчика (AI) "сердцебиения" */
		int maxHeartBeat;						/*! &lt; сохраняемое значение */
		
		xmlNode* confnode;
		/*! получить числовое свойство из конф. файла по привязанной confnode */
		int getIntProp(const std::string name) { return UniSetTypes::conf->getIntProp(confnode, name); }
		/*! получить текстовое свойство из конф. файла по привязанной confnode */
		inline const std::string getProp(const std::string name) { return UniSetTypes::conf->getProp(confnode, name); }

		int smReadyTimeout; 	/*!&lt; время ожидания готовности SM */
		bool activated;
		int activateTimeout;	/*!&lt; время ожидания готовности UniSetObject к работе */
		PassiveTimer ptStartUpTimeout;	/*!&lt; время на блокировку обработки WatchDog, если недавно был StartUp */
		int askPause; /*!&lt; пауза между неудачными попытками заказать датчики */
		
		IOController_i::SensorInfo si;
</xsl:template>

<xsl:template name="COMMON-HEAD-PRIVATE">

</xsl:template>

<xsl:template name="COMMON-CC-FILE">
void <xsl:value-of select="$CLASSNAME"/>_SK::processingMessage( UniSetTypes::VoidMessage* _msg )
{
	try
	{
		switch( _msg->type )
		{
			case Message::SensorInfo:
			{
				SensorMessage _sm( _msg );
				preSensorInfo( &amp;_sm );
				break;
			}

			case Message::Timer:
			{
				TimerMessage _tm(_msg);
				preTimerInfo(&amp;_tm);
				break;
			}

			case Message::SysCommand:
			{
				SystemMessage _sm( _msg );
				sysCommand( &amp;_sm );
				break;
			}

			default:
				break;
		}	
	}
	catch(Exception&amp; ex)
	{
		cout  &lt;&lt; myname &lt;&lt; "(processingMessage): " &lt;&lt; ex &lt;&lt; endl;
	}
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::sysCommand( SystemMessage* _sm )
{
	switch( _sm->command )
	{
		case SystemMessage::WatchDog:
			unideb &lt;&lt; myname &lt;&lt; "(sysCommand): WatchDog" &lt;&lt; endl;
			if( !active || !ptStartUpTimeout.checkTime() )
			{
				unideb[Debug::WARN] &lt;&lt; myname &lt;&lt; "(sysCommand): игнорируем WatchDog, потому-что только-что стартанули" &lt;&lt; endl;
				break;
			}
		case SystemMessage::StartUp:
		{
			waitSM(smReadyTimeout);
			ptStartUpTimeout.reset();
			updateOutputs(true); // принудительное обновление выходов
			updateValues();
			askSensors(UniversalIO::UIONotify);
			active = true;
			break;
		}
		
		case SystemMessage::FoldUp:
		case SystemMessage::Finish:
			askSensors(UniversalIO::UIODontNotify);
			break;
		
		case SystemMessage::LogRotate:
		{
			// переоткрываем логи
			unideb &lt;&lt; myname &lt;&lt; "(sysCommand): logRotate" &lt;&lt; endl;
			string fname = unideb.getLogFile();
			if( !fname.empty() )
			{
				unideb.logFile(fname.c_str());
				unideb &lt;&lt; myname &lt;&lt; "(sysCommand): ***************** UNIDEB LOG ROTATE *****************" &lt;&lt; endl;
			}

			unideb &lt;&lt; myname &lt;&lt; "(sysCommand): logRotate" &lt;&lt; endl;
			fname = unideb.getLogFile();
			if( !fname.empty() )
			{
				unideb.logFile(fname.c_str());
				unideb &lt;&lt; myname &lt;&lt; "(sysCommand): ***************** GGDEB LOG ROTATE *****************" &lt;&lt; endl;
			}
		}
		break;

		default:
			break;
	}
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::setState( UniSetTypes::ObjectId _sid, bool _state )
{
	setValue(_sid, _state ? 1 : 0 );
}
// -----------------------------------------------------------------------------
bool <xsl:value-of select="$CLASSNAME"/>_SK::checkTestMode()
{
	return (in_TestMode_S &amp;&amp; in_LocalTestMode_S);
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::sigterm( int signo )
{
	<xsl:if test="normalize-space($BASECLASS)!=''"><xsl:value-of select="normalize-space($BASECLASS)"/>::sigterm(signo);</xsl:if>
	<xsl:if test="normalize-space($BASECLASS)=''">UniSetObject::sigterm(signo);</xsl:if>
	active = false;
}
// -----------------------------------------------------------------------------
bool <xsl:value-of select="$CLASSNAME"/>_SK::activateObject()
{
	// блокирование обработки Startup 
	// пока не пройдёт инициализация датчиков
	// см. sysCommand()
	{
		activated = false;
		<xsl:if test="normalize-space($BASECLASS)!=''"><xsl:value-of select="normalize-space($BASECLASS)"/>::activateObject();</xsl:if>
		<xsl:if test="normalize-space($BASECLASS)=''">UniSetObject::activateObject();</xsl:if>
		activated = true;
	}

	return true;
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::preTimerInfo( UniSetTypes::TimerMessage* _tm )
{
	timerInfo(_tm);
}
// ----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::waitSM( int wait_msec, ObjectId _testID )
{
	if( _testID == DefaultObjectId )
		_testID = idTestMode_S;
		
	if( _testID == DefaultObjectId )
		return;
		
	if( unideb.debugging(Debug::INFO) )
	{
		unideb[Debug::INFO] &lt;&lt; myname &lt;&lt; "(waitSM): waiting SM ready " 
			&lt;&lt; wait_msec &lt;&lt; " msec"
			&lt;&lt; " testID=" &lt;&lt; _testID &lt;&lt; endl;
	}
		
	if( !ui.waitReady(_testID,wait_msec) )
	{
		ostringstream err;
		err &lt;&lt; myname 
			&lt;&lt; "(waitSM): Не дождались готовности(exist) SharedMemory к работе в течение " 
			&lt;&lt; wait_msec &lt;&lt; " мсек";

		unideb[Debug::CRIT] &lt;&lt; err.str() &lt;&lt; endl;
		terminate();
		abort();
		// kill(SIGTERM,getpid());	// прерываем (перезапускаем) процесс...
		throw SystemError(err.str());
	}

	if( idTestMode_S != DefaultObjectId )
	{
		if( !ui.waitWorking(idTestMode_S,wait_msec) )
		{
			ostringstream err;
			err &lt;&lt; myname 
				&lt;&lt; "(waitSM): Не дождались готовности(work) SharedMemory к работе в течение " 
				&lt;&lt; wait_msec &lt;&lt; " мсек";
	
			unideb[Debug::CRIT] &lt;&lt; err.str() &lt;&lt; endl;
			terminate();
			abort();
			// kill(SIGTERM,getpid());	// прерываем (перезапускаем) процесс...
			throw SystemError(err.str());
		}
	}
}
// ----------------------------------------------------------------------------
</xsl:template>

<xsl:template name="COMMON-ID-LIST">
	if( UniSetTypes::findArgParam("--print-id-list",conf->getArgc(),conf->getArgv()) != -1 )
	{
<xsl:for-each select="//smap/item">
		if( <xsl:value-of select="normalize-space(@name)"/> != UniSetTypes::DefaultObjectId )
    		cout &lt;&lt; "id:" &lt;&lt; <xsl:value-of select="normalize-space(@name)"/> &lt;&lt; endl;
</xsl:for-each>
<xsl:for-each select="//msgmap/item">
		if( <xsl:value-of select="normalize-space(@name)"/> != UniSetTypes::DefaultObjectId )
    		cout &lt;&lt; "id:" &lt;&lt; <xsl:value-of select="normalize-space(@name)"/> &lt;&lt; endl;
</xsl:for-each>
//		abort();
	}
</xsl:template>

<xsl:template name="COMMON-CC-HEAD">
// --------------------------------------------------------------------------
/*
 DO NOT EDIT THIS FILE. IT IS AUTOGENERATED FILE.
 ALL YOUR CHANGES WILL BE LOST.
 
 НЕ РЕДАКТИРУЙТЕ ЭТОТ ФАЙЛ. ЭТОТ ФАЙЛ СОЗДАН АВТОМАТИЧЕСКИ.
 ВСЕ ВАШИ ИЗМЕНЕНИЯ БУДУТ ПОТЕРЯНЫ.
*/ 
// --------------------------------------------------------------------------
// generate timestamp: <xsl:value-of select="date:date()"/>
// -----------------------------------------------------------------------------
#include <xsl:call-template name="preinclude"/>Configuration.h<xsl:call-template name="postinclude"/>
#include <xsl:call-template name="preinclude"/>Exceptions.h<xsl:call-template name="postinclude"/>
#include "<xsl:value-of select="$SK_H_FILENAME"/>"

// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
</xsl:template>

<xsl:template name="COMMON-CC-FUNCS">
// -----------------------------------------------------------------------------
<xsl:value-of select="$CLASSNAME"/>_SK::<xsl:value-of select="$CLASSNAME"/>_SK():
// Инициализация идентификаторов (имена берутся из конф. файла)
<xsl:for-each select="//smap/item">
	<xsl:value-of select="@name"/>(DefaultObjectId),
node_<xsl:value-of select="@name"/>(DefaultObjectId),
</xsl:for-each>
// Используемые идентификаторы сообщений (имена берутся из конф. файла)
<xsl:for-each select="//msgmap/item"><xsl:value-of select="@name"/>(DefaultObjectId),
node_<xsl:value-of select="@name"/>(DefaultObjectId),
</xsl:for-each>
active(false),
isTestMode(false),
idTestMode_S(DefaultObjectId),
idLocalTestMode_S(DefaultObjectId),
idHeartBeat(DefaultObjectId),
maxHeartBeat(10),
confnode(0),
smReadyTimeout(0),
activated(false),
askPause(2000)
{
	unideb[Debug::CRIT] &lt;&lt; "<xsl:value-of select="$CLASSNAME"/>: init failed!!!!!!!!!!!!!!!" &lt;&lt; endl;
	throw Exception( string(myname+": init failed!!!") );
}
// -----------------------------------------------------------------------------
<xsl:value-of select="$CLASSNAME"/>_SK::<xsl:value-of select="$CLASSNAME"/>_SK( ObjectId id, xmlNode* cnode ):
<xsl:if test="normalize-space($BASECLASS)!=''"><xsl:value-of select="normalize-space($BASECLASS)"/>(id),</xsl:if>
<xsl:if test="normalize-space($BASECLASS)=''">UniSetObject(id),</xsl:if>
// Инициализация идентификаторов (имена берутся из конф. файла)
<xsl:for-each select="//smap/item">
	<xsl:value-of select="normalize-space(@name)"/>(conf->getSensorID(conf->getProp(cnode,"<xsl:value-of select="normalize-space(@name)"/>"))),
node_<xsl:value-of select="normalize-space(@name)"/>( conf->getNodeID(conf->getProp(cnode,"node_<xsl:value-of select="normalize-space(@name)"/>")) ),
</xsl:for-each>
// Используемые идентификаторы сообщений (имена берутся из конф. файла)
<xsl:for-each select="//msgmap/item"><xsl:value-of select="normalize-space(@name)"/>(conf->getSensorID(conf->getProp(cnode,"<xsl:value-of select="normalize-space(@name)"/>"))),
node_<xsl:value-of select="normalize-space(@name)"/>(conf->getNodeID( conf->getProp(cnode,"node_<xsl:value-of select="normalize-space(@name)"/>"))),
</xsl:for-each>
sleep_msec(<xsl:call-template name="settings"><xsl:with-param name="varname" select="'sleep-msec'"/></xsl:call-template>),
active(true),
isTestMode(false),
idTestMode_S(conf->getSensorID("TestMode_S")),
idLocalTestMode_S(conf->getSensorID(conf->getProp(cnode,"LocalTestMode_S"))),
in_TestMode_S(false),
in_LocalTestMode_S(false),
idHeartBeat(DefaultObjectId),
maxHeartBeat(10),
confnode(cnode),
smReadyTimeout(0),
activated(false),
askPause(conf->getPIntProp(cnode,"askPause",2000))
{
	<xsl:call-template name="COMMON-ID-LIST"/>

<xsl:for-each select="//smap/item">
	<xsl:if test="normalize-space(@no_check_id)!='1'">
	if( <xsl:value-of select="normalize-space(@name)"/> == UniSetTypes::DefaultObjectId )
		throw Exception( myname + ": Not found ID for (<xsl:value-of select="@name"/>) " + conf->getProp(cnode,"<xsl:value-of select="@name"/>") );
	</xsl:if>
	
	if( node_<xsl:value-of select="normalize-space(@name)"/> == UniSetTypes::DefaultObjectId )
	{
		if( !conf->getProp(cnode,"node_<xsl:value-of select="normalize-space(@name)"/>").empty() )
			throw Exception( myname + ": Not found NodeID for (node='node_<xsl:value-of select="normalize-space(@name)"/>') " + conf->getProp(cnode,"node_<xsl:value-of select="normalize-space(@name)"/>") );
		
		node_<xsl:value-of select="normalize-space(@name)"/> = conf->getLocalNode();
	}
</xsl:for-each>

<xsl:for-each select="//msgmap/item">
	if( <xsl:value-of select="normalize-space(@name)"/> == UniSetTypes::DefaultObjectId )
	{
		if( !conf->getProp(cnode,"node_<xsl:value-of select="normalize-space(@name)"/>").empty() )
			throw Exception( myname + ": Not found Message::NodeID for (node='node_<xsl:value-of select="normalize-space(@name)"/>') " + conf->getProp(cnode,"node_<xsl:value-of select="normalize-space(@name)"/>") );
	}
	
	if( node_<xsl:value-of select="normalize-space(@name)"/> == UniSetTypes::DefaultObjectId )
	{
		if( !conf->getProp(cnode,"node_<xsl:value-of select="normalize-space(@name)"/>").empty() )
			throw Exception( myname + ": Not found Message::NodeID for (node='node_<xsl:value-of select="normalize-space(@name)"/>') " + conf->getProp(cnode,"node_<xsl:value-of select="normalize-space(@name)"/>") );

			node_<xsl:value-of select="normalize-space(@name)"/> = conf->getLocalNode();
	}
</xsl:for-each>

	UniXML_iterator it(cnode);
	string heart = conf->getArgParam("--heartbeat-id",it.getProp("heartbeat_id"));
	if( !heart.empty() )
	{
		idHeartBeat = conf->getSensorID(heart);
		if( idHeartBeat == DefaultObjectId )
		{
			ostringstream err;
			err &lt;&lt; myname &lt;&lt; ": не найден идентификатор для датчика 'HeartBeat' " &lt;&lt; heart;
			throw SystemError(err.str());
		}

		int heartbeatTime = conf->getArgPInt("--heartbeat-time",it.getProp("heartbeatTime"),conf-&gt;getHeartBeatTime());
		if( heartbeatTime>0 )
			ptHeartBeat.setTiming(heartbeatTime);
		else
			ptHeartBeat.setTiming(UniSetTimer::WaitUpTime);

		maxHeartBeat = conf->getArgPInt("--heartbeat-max",it.getProp("heartbeat_max"), 10);
	}

	// Инициализация значений
	<xsl:for-each select="//smap/item">
		<xsl:if test="normalize-space(@default)=''">
			<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = 0;
		</xsl:if>
		<xsl:if test="not(normalize-space(@default)='')">
			<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = <xsl:value-of select="@default"/>;
		</xsl:if>
	</xsl:for-each>
	
	<xsl:for-each select="//msgmap/item">
	<xsl:if test="normalize-space(@default)=''">m_<xsl:value-of select="@name"/> = 0;
	prev_m_<xsl:value-of select="@name"/> = 0;
	</xsl:if>
	<xsl:if test="not(normalize-space(@default)='')">m_<xsl:value-of select="@name"/> = <xsl:value-of select="@default"/>;
	prev_m_<xsl:value-of select="@name"/> = <xsl:value-of select="@default"/>;
	</xsl:if>
	</xsl:for-each>
	
	sleep_msec = conf->getArgPInt("--sleep-msec","<xsl:call-template name="settings"><xsl:with-param name="varname" select="'sleep-msec'"/></xsl:call-template>", <xsl:call-template name="settings"><xsl:with-param name="varname" select="'sleep-msec'"/></xsl:call-template>);

	resetMsgTime = conf->getPIntProp(cnode,"resetMsgTime", 2000);
	ptResetMsg.setTiming(resetMsgTime);

	smReadyTimeout = conf->getArgInt("--sm-ready-timeout","<xsl:call-template name="settings"><xsl:with-param name="varname" select="'smReadyTimeout'"/></xsl:call-template>");
	if( smReadyTimeout == 0 )
		smReadyTimeout = 60000;
	else if( smReadyTimeout &lt; 0 )
		smReadyTimeout = UniSetTimer::WaitUpTime;

	activateTimeout	= conf->getArgPInt("--activate-timeout", 20000);

	int msec = conf->getArgPInt("--startup-timeout", 10000);
	ptStartUpTimeout.setTiming(msec);

	// ===================== &lt;variables&gt; =====================
	<xsl:for-each select="//variables/item">
	std::string tmp_<xsl:value-of select="@name"/>( conf->getArgParam("--<xsl:value-of select="../@arg_prefix"/><xsl:value-of select="@name"/>",it.getProp("<xsl:value-of select="@name"/>")) );
	<xsl:if test="normalize-space(@default)!=''">if( tmp_<xsl:value-of select="@name"/>.empty() )
		tmp_<xsl:value-of select="@name"/> = "<xsl:value-of select="@default"/>";
	</xsl:if>

	<xsl:if test="normalize-space(@type)='int'">
	<xsl:value-of select="@name"/> = uni_atoi(tmp_<xsl:value-of select="@name"/>);
	</xsl:if>
	<xsl:if test="normalize-space(@type)='float'">
	<xsl:value-of select="@name"/> = atof(tmp_<xsl:value-of select="@name"/>.c_str());
	</xsl:if>
	<xsl:if test="normalize-space(@type)='bool'">
	<xsl:value-of select="@name"/> = uni_atoi(tmp_<xsl:value-of select="@name"/>);
	</xsl:if>
	<xsl:if test="normalize-space(@type)='str'">
	<xsl:value-of select="@name"/> = tmp_<xsl:value-of select="@name"/>;
	</xsl:if>
	<xsl:if test="normalize-space(@min)!=''">
	if( <xsl:value-of select="@name"/> &lt; <xsl:value-of select="@min"/> )
	{
		unideb[Debug::WARN] &lt;&lt; myname &lt;&lt; ": RANGE WARNING: <xsl:value-of select="@name"/>=" &lt;&lt; <xsl:value-of select="@name"/> &lt;&lt; " &lt; <xsl:value-of select="@min"/>" &lt;&lt; endl;
		<xsl:if test="normalize-space(@no_range_exception)=''">throw UniSetTypes::SystemError(myname+"(init): <xsl:value-of select="@name"/> &lt; <xsl:value-of select="@min"/>");</xsl:if>
	}
	</xsl:if>
	<xsl:if test="normalize-space(@max)!=''">
	if( <xsl:value-of select="@name"/> &gt; <xsl:value-of select="@max"/> )
	{
		unideb[Debug::WARN] &lt;&lt; myname &lt;&lt; ": RANGE WARNING: <xsl:value-of select="@name"/>=" &lt;&lt; <xsl:value-of select="@name"/> &lt;&lt; " &gt; <xsl:value-of select="@max"/>" &lt;&lt; endl;
		<xsl:if test="normalize-space(@no_range_exception)=''">throw UniSetTypes::SystemError(myname+"(init): <xsl:value-of select="@name"/> &gt; <xsl:value-of select="@max"/>");</xsl:if>
	}
	</xsl:if>
	// ----------
	</xsl:for-each>
	// ===================== end of &lt;variables&gt; =====================
}

// -----------------------------------------------------------------------------

<xsl:value-of select="$CLASSNAME"/>_SK::~<xsl:value-of select="$CLASSNAME"/>_SK()
{
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::updateValues()
{
	// Опрашиваем все входы...
	in_TestMode_S  		= (idTestMode_S!=DefaultObjectId) ? ui.getState(idTestMode_S):false;
	in_LocalTestMode_S 	= (idLocalTestMode_S!=DefaultObjectId) ? ui.getState(idLocalTestMode_S):false;
	
		<xsl:for-each select="//smap/item">
			<xsl:choose>
				<xsl:when test="normalize-space(@vartype)='in'"><xsl:call-template name="getdata"/></xsl:when>
				<xsl:when test="normalize-space(@vartype)='io'"><xsl:call-template name="getdata"/></xsl:when>
			</xsl:choose>
		</xsl:for-each>
<!--
	<xsl:for-each select="//smap/item">
		<xsl:choose>
				<xsl:when test="normalize-space(@vartype)='in'"><xsl:call-template name="getdata"><xsl:with-param name="output" select="1"/></xsl:call-template></xsl:when>
				<xsl:when test="normalize-space(@vartype)='io'"><xsl:call-template name="getdata"><xsl:with-param name="output" select="1"/></xsl:call-template></xsl:when>
		</xsl:choose>
	</xsl:for-each>
-->	
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::updatePreviousValues()
{
	<xsl:for-each select="//smap/item">prev_<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>;
	</xsl:for-each>
	
	<xsl:for-each select="//msgmap/item">prev_m_<xsl:value-of select="@name"/> = m_<xsl:value-of select="@name"/>;
	</xsl:for-each>
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::checkSensors()
{
	<xsl:for-each select="//smap/item">
		<xsl:choose>
			<xsl:when test="normalize-space(@vartype)='in'"><xsl:call-template name="check_changes"/></xsl:when>
			<xsl:when test="normalize-space(@vartype)='io'"><xsl:call-template name="check_changes"/></xsl:when>
		</xsl:choose>
	</xsl:for-each>
}
// -----------------------------------------------------------------------------
bool <xsl:value-of select="$CLASSNAME"/>_SK::alarm( UniSetTypes::ObjectId _code, bool _state )
{
	if( _code == UniSetTypes::DefaultObjectId )
	{
		unideb[Debug::CRIT]  &lt;&lt; getName()
							&lt;&lt; "(alarm): попытка послать сообщение с DefaultObjectId" 
							&lt;&lt; endl;
		return false;	
	}

	unideb[Debug::LEVEL1]  &lt;&lt; getName()  &lt;&lt; "(alarm): ";
	if( _state )
		unideb(Debug::LEVEL1) &lt;&lt; "SEND ";
	else
		unideb(Debug::LEVEL1) &lt;&lt; "RESET ";
	
	unideb(Debug::LEVEL1) &lt;&lt; endl;
	
	<xsl:for-each select="//msgmap/item">
	if( _code == <xsl:value-of select="@name"/> )
	{				
		unideb[Debug::LEVEL1] &lt;&lt; "<xsl:value-of select="@name"/>" &lt;&lt; endl;
		try
		{
			m_<xsl:value-of select="@name"/> = _state;
			// сохраняем сразу...
			si.id = <xsl:value-of select="@name"/>;
			si.node = node_<xsl:value-of select="@name"/>;
			ui.saveState( si,m_<xsl:value-of select="@name"/>,UniversalIO::DigitalInput,getId() );
			return true;
		}
		catch(...){}
		return false;
	}
	</xsl:for-each>
	
	unideb[Debug::LEVEL1] &lt;&lt; " not found MessgeOID?!!" &lt;&lt; endl;
	return false;
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::resetMsg()
{
// reset messages
<xsl:for-each select="//msgmap/item">
	m_<xsl:value-of select="@name"/> = 0;
	if( <xsl:value-of select="@name"/> != UniSetTypes::DefaultObjectId )
	{
		try
		{
			si.id = <xsl:value-of select="@name"/>;
			si.node = node_<xsl:value-of select="@name"/>;
			ui.saveState( si,false,UniversalIO::DigitalInput,getId() );
		}
		catch( UniSetTypes::Exception&amp; ex )
		{
			unideb[Debug::LEVEL1] &lt;&lt; getName() &lt;&lt; ex &lt;&lt; endl;
		}
	}
</xsl:for-each>
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::testMode( bool _state )
{
	if( !_state  )
		return;

	// отключаем все выходы
	<xsl:for-each select="//smap/item">
	<xsl:choose>
		<xsl:when test="normalize-space(@vartype)='out'"><xsl:call-template name="setdata_value"><xsl:with-param name="setval" select="0"/></xsl:call-template></xsl:when>
		<xsl:when test="normalize-space(@vartype)='io'"><xsl:call-template name="setdata_value"><xsl:with-param name="setval" select="0"/></xsl:call-template></xsl:when>
	</xsl:choose>
	</xsl:for-each>
}
// -----------------------------------------------------------------------------
</xsl:template>


<xsl:template name="COMMON-CC-ALONE-FUNCS">
// -----------------------------------------------------------------------------
<xsl:value-of select="$CLASSNAME"/>_SK::<xsl:value-of select="$CLASSNAME"/>_SK():
<xsl:for-each select="//sensors/item">
	<xsl:call-template name="setvar">
		<xsl:with-param name="GENTYPE" select="'C'"/>
	</xsl:call-template>
</xsl:for-each>
<xsl:for-each select="//sensors/item">
	<xsl:call-template name="setmsg">
		<xsl:with-param name="GENTYPE" select="'C'"/>
	</xsl:call-template>
</xsl:for-each>
active(false),
isTestMode(false),
idTestMode_S(DefaultObjectId),
idLocalTestMode_S(DefaultObjectId),
idHeartBeat(DefaultObjectId),
maxHeartBeat(10),
confnode(0),
activated(false),
askPause(2000)
{
	unideb[Debug::CRIT] &lt;&lt; "<xsl:value-of select="$CLASSNAME"/>: init failed!!!!!!!!!!!!!!!" &lt;&lt; endl;
	throw Exception( string(myname+": init failed!!!") );
}
// -----------------------------------------------------------------------------
<xsl:value-of select="$CLASSNAME"/>_SK::<xsl:value-of select="$CLASSNAME"/>_SK( ObjectId id, xmlNode* cnode ):
<xsl:if test="normalize-space($BASECLASS)!=''"><xsl:value-of select="normalize-space($BASECLASS)"/>(id),</xsl:if>
<xsl:if test="normalize-space($BASECLASS)=''">UniSetObject(id),</xsl:if>
// Инициализация идентификаторов (имена берутся из конф. файла)
<xsl:for-each select="//sensors/item">
	<xsl:call-template name="setvar">
		<xsl:with-param name="GENTYPE" select="'C'"/>
	</xsl:call-template>
</xsl:for-each>
// Используемые идентификаторы сообщений (имена берутся из конф. файла)
<xsl:for-each select="//sensors/item">
	<xsl:call-template name="setmsg">
		<xsl:with-param name="GENTYPE" select="'C'"/>
	</xsl:call-template>
</xsl:for-each>
sleep_msec(<xsl:call-template name="settings-alone"><xsl:with-param name="varname" select="'sleep-msec'"/></xsl:call-template>),
active(true),
isTestMode(false),
idTestMode_S(conf->getSensorID("TestMode_S")),
idLocalTestMode_S(conf->getSensorID(conf->getProp(cnode,"LocalTestMode_S"))),
in_TestMode_S(false),
in_LocalTestMode_S(false),
idHeartBeat(DefaultObjectId),
maxHeartBeat(10),
confnode(cnode),
activated(false),
askPause(conf->getPIntProp(cnode,"askPause",2000))
{
	si.node = conf->getLocalNode();

<xsl:for-each select="//sensors/item">
	<xsl:call-template name="setmsg">
		<xsl:with-param name="GENTYPE" select="'CHECK'"/>
	</xsl:call-template>
</xsl:for-each>

	UniXML_iterator it(cnode);
	string heart = conf->getArgParam("--heartbeat-id",it.getProp("heartbeat_id"));
	if( !heart.empty() )
	{
		idHeartBeat = conf->getSensorID(heart);
		if( idHeartBeat == DefaultObjectId )
		{
			ostringstream err;
			err &lt;&lt; myname &lt;&lt; ": не найден идентификатор для датчика 'HeartBeat' " &lt;&lt; heart;
			throw SystemError(err.str());
		}

		int heartbeatTime = conf->getArgPInt("--heartbeat-time",it.getProp("heartbeatTime"),conf-&gt;getHeartBeatTime());

		if( heartbeatTime>0 )
			ptHeartBeat.setTiming(heartbeatTime);
		else
			ptHeartBeat.setTiming(UniSetTimer::WaitUpTime);

		maxHeartBeat = conf->getArgPInt("--heartbeat-max",it.getProp("heartbeat_max"), 10);
	}


	sleep_msec = conf->getArgPInt("--sleep-msec","<xsl:call-template name="settings-alone"><xsl:with-param name="varname" select="'sleep-msec'"/></xsl:call-template>", <xsl:call-template name="settings-alone"><xsl:with-param name="varname" select="'sleep-msec'"/></xsl:call-template>);

	resetMsgTime = conf->getPIntProp(cnode,"resetMsgTime", 2000);
	ptResetMsg.setTiming(resetMsgTime);

	smReadyTimeout = conf->getArgInt("--sm-ready-timeout","<xsl:call-template name="settings"><xsl:with-param name="varname" select="'smReadyTimeout'"/></xsl:call-template>");
	if( smReadyTimeout == 0 )
		smReadyTimeout = 60000;
	else if( smReadyTimeout &lt; 0 )
		smReadyTimeout = UniSetTimer::WaitUpTime;

	activateTimeout	= conf->getArgPInt("--activate-timeout", 20000);

	int msec = conf->getArgPInt("--startup-timeout", 10000);
	ptStartUpTimeout.setTiming(msec);
}

// -----------------------------------------------------------------------------

<xsl:value-of select="$CLASSNAME"/>_SK::~<xsl:value-of select="$CLASSNAME"/>_SK()
{
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::updateValues()
{
	in_TestMode_S  		= (idTestMode_S!=DefaultObjectId) ? ui.getState(idTestMode_S):false;
	in_LocalTestMode_S 	= (idLocalTestMode_S!=DefaultObjectId) ? ui.getState(idLocalTestMode_S):false;

	// Опрашиваем все входы...
	<xsl:for-each select="//sensors/item/consumers/consumer">
		<xsl:choose>
			<xsl:when test="normalize-space(@vartype)='in'"><xsl:call-template name="getdata"/></xsl:when>
			<xsl:when test="normalize-space(@vartype)='io'"><xsl:call-template name="getdata"/></xsl:when>
		</xsl:choose>
	</xsl:for-each>
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::testMode( bool _state )
{
	// отключаем все выходы
<xsl:for-each select="//sensors/item/consumers/consumer">
	<xsl:call-template name="setdata_value"><xsl:with-param name="setval" select="0"/></xsl:call-template>
</xsl:for-each>
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::checkSensors()
{
	// Опрашиваем все входы...
	<xsl:for-each select="//sensors/item/consumers/consumer">
		<xsl:choose>
			<xsl:when test="normalize-space(@vartype)='in'"><xsl:call-template name="check_changes"/></xsl:when>
			<xsl:when test="normalize-space(@vartype)='io'"><xsl:call-template name="check_changes"/></xsl:when>
		</xsl:choose>
	</xsl:for-each>
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::updatePreviousValues()
{
<xsl:for-each select="//sensors/item/consumers/consumer">
<xsl:if test="normalize-space(../../@msg)!='1'">
<xsl:if test="normalize-space(@name)=$OID">	prev_<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/> = <xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>;
</xsl:if>
</xsl:if>
</xsl:for-each>

<xsl:for-each select="//sensors/item/consumers/consumer">
<xsl:if test="normalize-space(../../@msg)='1'">
<xsl:if test="normalize-space(@name)=$OID">	prev_m_<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/> = m_<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>;
</xsl:if>
</xsl:if>
</xsl:for-each>

}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::resetMsg()
{
// reset messages
<xsl:for-each select="//sensors/item">
	<xsl:call-template name="setmsg">
		<xsl:with-param name="GENTYPE" select="'R'"/>
	</xsl:call-template>
</xsl:for-each>

}
// -----------------------------------------------------------------------------
bool <xsl:value-of select="$CLASSNAME"/>_SK::alarm( UniSetTypes::ObjectId _code, bool _state )
{
	if( _code == UniSetTypes::DefaultObjectId )
	{
		unideb[Debug::CRIT]  &lt;&lt; getName()
							&lt;&lt; "(alarm): попытка послать сообщение с DefaultObjectId" 
							&lt;&lt; endl;
		return false;	
	}

	unideb[Debug::LEVEL1]  &lt;&lt; getName()  &lt;&lt; "(alarm): ";
	if( _state )
		unideb(Debug::LEVEL1) &lt;&lt; "SEND (" &lt;&lt; _code &lt;&lt; ")";
	else
		unideb(Debug::LEVEL1) &lt;&lt; "RESET (" &lt;&lt; _code &lt;&lt; ")";


<xsl:for-each select="//sensors/item">
	<xsl:call-template name="setmsg">
		<xsl:with-param name="GENTYPE" select="'A'"/>
	</xsl:call-template>
</xsl:for-each>
	
	unideb(Debug::LEVEL8) &lt;&lt; " not found MessgeOID?!!" &lt;&lt; endl;
	return false;
}
// -----------------------------------------------------------------------------
</xsl:template>

</xsl:stylesheet>
