<?xml version='1.0' encoding="utf-8" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version='1.0'
		             xmlns:date="http://exslt.org/dates-and-times">

<xsl:output method="text" indent="yes" encoding="utf-8"/>

<xsl:template name="setprefix">
<xsl:choose>
	<xsl:when test="normalize-space(@vartype)='in'">in_</xsl:when>
	<xsl:when test="normalize-space(@vartype)='out'">out_</xsl:when>
	<xsl:when test="normalize-space(@vartype)='none'">nn_</xsl:when>
	<xsl:when test="normalize-space(@vartype)='io'">NOTSUPPORTED_IO_VARTYPE_</xsl:when>
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
		<xsl:if test="normalize-space(@vartype)!='io'">const UniSetTypes::ObjectId <xsl:value-of select="../../@name"/>; 		/*!&lt; <xsl:value-of select="../../@textname"/> */
		UniSetTypes::ObjectId node_<xsl:value-of select="../../@name"/>;
		long <xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>; /*!&lt; текущее значение '<xsl:value-of select="../../@name"/>' */
		</xsl:if>
		<xsl:if test="normalize-space(@vartype)='io'">#warning (uniset-codegen): vartype='io' NO LONGER SUPPORTED! (ignore variable: '<xsl:value-of select="../../@name"/>')
		</xsl:if>
		</xsl:when>
		<xsl:when test="$GENTYPE='H-PRIVATE'">
		<xsl:if test="normalize-space(@vartype)!='io'">long prev_<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>; /*!&lt; предыдущее значение '<xsl:value-of select="../../@name"/>'*/
		</xsl:if>
		<xsl:if test="normalize-space(@vartype)='io'">#warning (uniset-codegen): vartype='io' NO LONGER SUPPORTED! (ignore variable: '<xsl:value-of select="../../@name"/>')
		</xsl:if>
		</xsl:when>
		<xsl:when test="$GENTYPE='C'">
		<xsl:if test="normalize-space(@vartype)!='io'">
		        <xsl:value-of select="../../@name"/>(<xsl:value-of select="../../@id"/>),
			<xsl:if test="not(normalize-space(../../@node)='')">
				node_<xsl:value-of select="../../@name"/>(uniset_conf()->getNodeID("<xsl:value-of select="../../@node"/>")),
			</xsl:if>
			<xsl:if test="normalize-space(../../@node)=''">
				node_<xsl:value-of select="../../@name"/>(UniSetTypes::uniset_conf()->getLocalNode()),
			</xsl:if>
			<xsl:if test="normalize-space(../../@default)=''">
				<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>(0),
			</xsl:if>
			<xsl:if test="not(normalize-space(../../@default)='')">
				<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>(<xsl:value-of select="../../@default"/>),
			</xsl:if>
		</xsl:if>
		</xsl:when>
		<xsl:when test="$GENTYPE='CHECK'">
		<xsl:if test="normalize-space(@vartype)!='io'">
			<xsl:if test="normalize-space(../../@id)=''">throw SystemException("Not Found ID for <xsl:value-of select="../../@name"/>");</xsl:if>
		</xsl:if>
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
			</xsl:when>
			<xsl:when test="$GENTYPE='H-PRIVATE'">bool prev_m_<xsl:value-of select="normalize-space(../../@name)"/>; 	/*!&lt; предыдущее состояние (сообщения) */
			</xsl:when>
			<xsl:when test="$GENTYPE='C'">mid_<xsl:value-of select="normalize-space(../../@name)"/>(<xsl:value-of select="../../@id"/>),
				m_<xsl:value-of select="normalize-space(../../@name)"/>(false),
				prev_m_<xsl:value-of select="normalize-space(../../@name)"/>(false),
			</xsl:when>
			<xsl:when test="$GENTYPE='CHECK'">
				<xsl:if test="normalize-space(@no_check_id)!='1'">
				<xsl:if test="normalize-space(../../@id)=''">uwarn &lt;&lt; myname &lt;&lt; ": Not found (Message)OID for mid_<xsl:value-of select="normalize-space(../../@name)"/>" &lt;&lt; endl;
				</xsl:if>
				</xsl:if>
			</xsl:when>
			<xsl:when test="$GENTYPE='U'">
				si.id 	= mid_<xsl:value-of select="../../@name"/>;
				ui->setValue( si,( m_<xsl:value-of select="../../@name"/> ? 1:0), getId() );
			</xsl:when>		
			<xsl:when test="$GENTYPE='A'">
			if( _code == mid_<xsl:value-of select="../../@name"/> )
			{				
                ulog8 &lt;&lt; "<xsl:value-of select="../../@name"/>" &lt;&lt; endl;
				m_<xsl:value-of select="../../@name"/> = _state;
				try
				{
					// сохраняем сразу...
					si.id = mid_<xsl:value-of select="../../@name"/>;
					ui->setValue( si,(m_<xsl:value-of select="../../@name"/> ? 1:0), getId() );
					return true;
				}
			    catch( std::exception&amp;ex )
			    {
			        ucrit &lt;&lt; myname &lt;&lt; "(execute): catch " &lt;&lt; ex.what()  &lt;&lt;   endl;
			    }
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
						ui->setValue( si,0,getId() );
					}
					catch( UniSetTypes::Exception&amp; ex )
					{
                        ulog1 &lt;&lt; getName() &lt;&lt; ex &lt;&lt; endl;
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
		long getValue( UniSetTypes::ObjectId sid );
		void setValue( UniSetTypes::ObjectId sid, long value );
		void askSensor( UniSetTypes::ObjectId sid, UniversalIO::UIOCommand, UniSetTypes::ObjectId node = UniSetTypes::uniset_conf()->getLocalNode() );
		void updateValues();
		void setMsg( UniSetTypes::ObjectId code, bool state );

		DebugStream mylog;
		void init_dlog( DebugStream&amp; d );

        // "синтаксический сахар"..для логов
        #define myinfo if( mylog.debugging(Debug::INFO) ) mylog
        #define mywarn if( mylog.debugging(Debug::WARN) ) mylog
        #define mycrit if( mylog.debugging(Debug::CRIT) ) mylog
        #define mylog1 if( mylog.debugging(Debug::LEVEL1) ) mylog
        #define mylog2 if( mylog.debugging(Debug::LEVEL2) ) mylog
        #define mylog3 if( mylog.debugging(Debug::LEVEL3) ) mylog
        #define mylog4 if( mylog.debugging(Debug::LEVEL4) ) mylog
        #define mylog5 if( mylog.debugging(Debug::LEVEL5) ) mylog
        #define mylog6 if( mylog.debugging(Debug::LEVEL6) ) mylog
        #define mylog7 if( mylog.debugging(Debug::LEVEL7) ) mylog
        #define mylog8 if( mylog.debugging(Debug::LEVEL8) ) mylog
        #define mylog9 if( mylog.debugging(Debug::LEVEL9) ) mylog
</xsl:template>

<xsl:template name="COMMON-HEAD-PROTECTED">
		virtual void callback() override;
		virtual void processingMessage( UniSetTypes::VoidMessage* msg ) override;
		virtual void sysCommand( const UniSetTypes::SystemMessage* sm ) override;
		virtual void askSensors( UniversalIO::UIOCommand cmd ){}
		virtual void sensorInfo( const UniSetTypes::SensorMessage* sm ) override{}
		virtual void timerInfo( const UniSetTypes::TimerMessage* tm ) override{}
		virtual void sigterm( int signo ) override;
		virtual bool activateObject() override;
		virtual void testMode( bool state );
		void updatePreviousValues();
		void checkSensors();
		void updateOutputs( bool force );
<xsl:if test="normalize-space($TESTMODE)!=''">
		bool checkTestMode();
</xsl:if>
		void preAskSensors( UniversalIO::UIOCommand cmd );
		void preSensorInfo( const UniSetTypes::SensorMessage* sm );
		void preTimerInfo( const UniSetTypes::TimerMessage* tm );
		void waitSM( int wait_msec, UniSetTypes::ObjectId testID = UniSetTypes::DefaultObjectId );

		void resetMsg();
		Trigger trResetMsg;
		PassiveTimer ptResetMsg;
		int resetMsgTime;

		// Выполнение очередного шага программы
		virtual void step(){}

		int sleep_msec; /*!&lt; пауза между итерациями */
		bool active;
<xsl:if test="normalize-space($TESTMODE)!=''">
		bool isTestMode;
		Trigger trTestMode;
		UniSetTypes::ObjectId idTestMode_S;		  	/*!&lt; идентификатор для флага тестовго режима (для всех) */
		UniSetTypes::ObjectId idLocalTestMode_S;	/*!&lt; идентификатор для флага тестовго режима (для данного узла) */
		bool in_TestMode_S;
		bool in_LocalTestMode_S;
</xsl:if>
		UniSetTypes::ObjectId smTestID; /*!&lt; идентификатор датчика для тестирования готовности SM */

		// управление датчиком "сердцебиения"
		PassiveTimer ptHeartBeat;				/*! &lt; период "сердцебиения" */
		UniSetTypes::ObjectId idHeartBeat;		/*! &lt; идентификатор датчика (AI) "сердцебиения" */
		int maxHeartBeat;						/*! &lt; сохраняемое значение */
		
		xmlNode* confnode;
		/*! получить числовое свойство из конф. файла по привязанной confnode */
		int getIntProp(const std::string&amp; name) { return UniSetTypes::uniset_conf()->getIntProp(confnode, name); }
		/*! получить текстовое свойство из конф. файла по привязанной confnode */
		inline const std::string getProp(const std::string&amp; name) { return UniSetTypes::uniset_conf()->getProp(confnode, name); }

		int smReadyTimeout; 	/*!&lt; время ожидания готовности SM */
		std::atomic_bool activated;
		int activateTimeout;	/*!&lt; время ожидания готовности UniSetObject к работе */
		PassiveTimer ptStartUpTimeout;	/*!&lt; время на блокировку обработки WatchDog, если недавно был StartUp */
		int askPause; /*!&lt; пауза между неудачными попытками заказать датчики */
		
		IOController_i::SensorInfo si;
		bool forceOut; /*!&lt; флаг принудительного обноления "выходов" */
</xsl:template>

<xsl:template name="COMMON-HEAD-PRIVATE">

</xsl:template>

<xsl:template name="COMMON-CC-FILE">
// ------------------------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::init_dlog( DebugStream&amp; d )
{
	<xsl:value-of select="$CLASSNAME"/>_SK::mylog = d;
}
// ------------------------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::processingMessage( UniSetTypes::VoidMessage* _msg )
{
	try
	{
		switch( _msg->type )
		{
			case Message::SensorInfo:
				preSensorInfo( reinterpret_cast&lt;SensorMessage*&gt;(_msg) );
			break;

			case Message::Timer:
				preTimerInfo( reinterpret_cast&lt;TimerMessage*&gt;(_msg) );
			break;

			case Message::SysCommand:
				sysCommand( reinterpret_cast&lt;SystemMessage*&gt;(_msg) );
			break;

			default:
				break;
		}	
	}
	catch( Exception&amp; ex )
	{
		ucrit  &lt;&lt; myname &lt;&lt; "(processingMessage): " &lt;&lt; ex &lt;&lt; endl;
	}
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::sysCommand( const SystemMessage* _sm )
{
	switch( _sm->command )
	{
		case SystemMessage::WatchDog:
			ulog &lt;&lt; myname &lt;&lt; "(sysCommand): WatchDog" &lt;&lt; endl;
			if( !active || !ptStartUpTimeout.checkTime() )
			{
                uwarn &lt;&lt; myname &lt;&lt; "(sysCommand): игнорируем WatchDog, потому-что только-что стартанули" &lt;&lt; endl;
				break;
			}
		case SystemMessage::StartUp:
		{
			waitSM(smReadyTimeout);
			ptStartUpTimeout.reset();
			// т.к. для io-переменных важно соблюдать последовательность!
			// сперва обновить входы.. а потом уже выходы
			updateValues();
			updateOutputs(true); // принудительное обновление выходов
			preAskSensors(UniversalIO::UIONotify);
			askSensors(UniversalIO::UIONotify);
			active = true;
			break;
		}
		
		case SystemMessage::FoldUp:
		case SystemMessage::Finish:
			preAskSensors(UniversalIO::UIODontNotify);
			askSensors(UniversalIO::UIODontNotify);
			break;
		
		case SystemMessage::LogRotate:
		{
			// переоткрываем логи
			mylog &lt;&lt; myname &lt;&lt; "(sysCommand): logRotate" &lt;&lt; endl;
			string fname( mylog.getLogFile() );
			if( !fname.empty() )
			{
				mylog.logFile(fname.c_str(),true);
				mylog &lt;&lt; myname &lt;&lt; "(sysCommand): ***************** mylog LOG ROTATE *****************" &lt;&lt; endl;
			}
		}
		break;

		default:
			break;
	}
}
// -----------------------------------------------------------------------------
<xsl:if test="normalize-space($TESTMODE)!=''">
bool <xsl:value-of select="$CLASSNAME"/>_SK::checkTestMode()
{
	return (in_TestMode_S &amp;&amp; in_LocalTestMode_S);
}
// -----------------------------------------------------------------------------
</xsl:if>
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
void <xsl:value-of select="$CLASSNAME"/>_SK::preTimerInfo( const UniSetTypes::TimerMessage* _tm )
{
	timerInfo(_tm);
}
// ----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::waitSM( int wait_msec, ObjectId _testID )
{
<xsl:if test="normalize-space($TESTMODE)!=''">
	if( _testID == DefaultObjectId )
		_testID = idTestMode_S;
</xsl:if>

	if( _testID == DefaultObjectId )
		_testID = smTestID;

	if( _testID == DefaultObjectId )
		return;
		
	uinfo &lt;&lt; myname &lt;&lt; "(waitSM): waiting SM ready "
			&lt;&lt; wait_msec &lt;&lt; " msec"
			&lt;&lt; " testID=" &lt;&lt; _testID &lt;&lt; endl;
		
	if( !ui->waitReady(_testID,wait_msec) )
	{
		ostringstream err;
		err &lt;&lt; myname 
			&lt;&lt; "(waitSM): Не дождались готовности(exist) SharedMemory к работе в течение " 
			&lt;&lt; wait_msec &lt;&lt; " мсек";

        ucrit &lt;&lt; err.str() &lt;&lt; endl;
//		terminate();
//		abort();
		raise(SIGTERM);
		terminate();
//		throw SystemError(err.str());
	}

<xsl:if test="normalize-space($TESTMODE)!=''">
	if( idTestMode_S != DefaultObjectId )
	{
		if( !ui->waitWorking(idTestMode_S,wait_msec) )
		{
			ostringstream err;
			err &lt;&lt; myname 
				&lt;&lt; "(waitSM): Не дождались готовности(work) SharedMemory к работе в течение " 
				&lt;&lt; wait_msec &lt;&lt; " мсек";
	
            ucrit &lt;&lt; err.str() &lt;&lt; endl;
//			terminate();
//			abort();
			raise(SIGTERM);
//			throw SystemError(err.str());
		}
	}
</xsl:if>
}
// ----------------------------------------------------------------------------
</xsl:template>

<xsl:template name="COMMON-ID-LIST">
	if( UniSetTypes::findArgParam("--print-id-list",uniset_conf()->getArgc(),uniset_conf()->getArgv()) != -1 )
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

<xsl:template name="default-init-variables">
<xsl:if test="normalize-space(@const)!=''">
<xsl:if test="normalize-space(@type)='int'"><xsl:value-of select="normalize-space(@name)"/>(0),</xsl:if>
<xsl:if test="normalize-space(@type)='long'"><xsl:value-of select="normalize-space(@name)"/>(0),</xsl:if>
<xsl:if test="normalize-space(@type)='float'"><xsl:value-of select="normalize-space(@name)"/>(0),</xsl:if>
<xsl:if test="normalize-space(@type)='double'"><xsl:value-of select="normalize-space(@name)"/>(0),</xsl:if>
<xsl:if test="normalize-space(@type)='bool'"><xsl:value-of select="normalize-space(@name)"/>(false),</xsl:if>
<xsl:if test="normalize-space(@type)='str'"><xsl:value-of select="normalize-space(@name)"/>(""),</xsl:if>
</xsl:if>
</xsl:template>
<xsl:template name="init-variables">
<xsl:if test="normalize-space(@type)='int'">
<xsl:value-of select="normalize-space(@name)"/>(uni_atoi( init3_str(uniset_conf()->getArgParam("--" + argprefix + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>"))),
</xsl:if>
<xsl:if test="normalize-space(@type)='long'">
<xsl:value-of select="normalize-space(@name)"/>(uni_atoi( init3_str(uniset_conf()->getArgParam("--" + argprefix + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>"))),
</xsl:if>
<xsl:if test="normalize-space(@type)='float'">
<xsl:value-of select="normalize-space(@name)"/>(atof( init3_str(uniset_conf()->getArgParam("--" + argprefix + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>").c_str())),
</xsl:if>
<xsl:if test="normalize-space(@type)='double'">
<xsl:value-of select="normalize-space(@name)"/>(atof( init3_str(uniset_conf()->getArgParam("--" + argprefix + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>").c_str())),
</xsl:if>
<xsl:if test="normalize-space(@type)='bool'">
<xsl:value-of select="normalize-space(@name)"/>(uni_atoi( init3_str(uniset_conf()->getArgParam("--" + argprefix + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>"))),
</xsl:if>
<xsl:if test="normalize-space(@type)='str'">
<xsl:value-of select="normalize-space(@name)"/>(init3_str(uniset_conf()->getArgParam("--" + argprefix + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>")),
</xsl:if>
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
<xsl:if test="normalize-space(@vartype)!='io'">
	<xsl:value-of select="@name"/>(DefaultObjectId),
node_<xsl:value-of select="@name"/>(DefaultObjectId),
</xsl:if>
</xsl:for-each>
// Используемые идентификаторы сообщений (имена берутся из конф. файла)
<xsl:for-each select="//msgmap/item"><xsl:value-of select="@name"/>(DefaultObjectId),
node_<xsl:value-of select="@name"/>(DefaultObjectId),
m_<xsl:value-of select="@name"/>(false),
prev_m_<xsl:value-of select="@name"/>(false),
</xsl:for-each>
// variables
<xsl:for-each select="//variables/item">
<xsl:if test="normalize-space(@public)!=''">
<xsl:call-template name="default-init-variables"/>
</xsl:if>
<xsl:if test="normalize-space(@private)=''">
<xsl:if test="normalize-space(@public)=''">
<xsl:call-template name="default-init-variables"/>
</xsl:if>
</xsl:if>
</xsl:for-each>
active(false),
<xsl:if test="normalize-space($TESTMODE)!=''">
isTestMode(false),
idTestMode_S(DefaultObjectId),
idLocalTestMode_S(DefaultObjectId),
</xsl:if>
idHeartBeat(DefaultObjectId),
maxHeartBeat(10),
confnode(0),
smReadyTimeout(0),
activated(false),
askPause(2000),
forceOut(false),
<xsl:for-each select="//variables/item">
<xsl:if test="normalize-space(@private)!=''">
<xsl:call-template name="default-init-variables"/>
</xsl:if>
</xsl:for-each>
end_private(false)
{
	ucrit &lt;&lt; "<xsl:value-of select="$CLASSNAME"/>: init failed!!!!!!!!!!!!!!!" &lt;&lt; endl;
	throw Exception( string(myname+": init failed!!!") );
}
// -----------------------------------------------------------------------------
// ( val, confval, default val )
static const std::string init3_str( const std::string&amp; s1, const std::string&amp; s2, const std::string&amp; s3 )
{
	if( !s1.empty() )
		return s1;
	if( !s2.empty() )
		return s2;
	
	return s3;
}
// -----------------------------------------------------------------------------
<xsl:value-of select="$CLASSNAME"/>_SK::<xsl:value-of select="$CLASSNAME"/>_SK( ObjectId id, xmlNode* cnode, const std::string&amp; argprefix ):
<xsl:if test="normalize-space($BASECLASS)!=''"><xsl:value-of select="normalize-space($BASECLASS)"/>(id),</xsl:if>
<xsl:if test="normalize-space($BASECLASS)=''">UniSetObject(id),</xsl:if>
// Инициализация идентификаторов (имена берутся из конф. файла)
<xsl:for-each select="//smap/item">
<xsl:if test="normalize-space(@vartype)!='io'">
	<xsl:value-of select="normalize-space(@name)"/>(uniset_conf()->getSensorID(uniset_conf()->getProp(cnode,"<xsl:value-of select="normalize-space(@name)"/>"))),
node_<xsl:value-of select="normalize-space(@name)"/>( uniset_conf()->getNodeID(uniset_conf()->getProp(cnode,"node_<xsl:value-of select="normalize-space(@name)"/>")) ),
</xsl:if>
</xsl:for-each>
// Используемые идентификаторы сообщений (имена берутся из конф. файла)
<xsl:for-each select="//msgmap/item"><xsl:value-of select="normalize-space(@name)"/>(uniset_conf()->getSensorID(uniset_conf()->getProp(cnode,"<xsl:value-of select="normalize-space(@name)"/>"))),
node_<xsl:value-of select="normalize-space(@name)"/>(uniset_conf()->getNodeID( uniset_conf()->getProp(cnode,"node_<xsl:value-of select="normalize-space(@name)"/>"))),
m_<xsl:value-of select="normalize-space(@name)"/>(false),
prev_m_<xsl:value-of select="normalize-space(@name)"/>(false),
</xsl:for-each>
// variables
<xsl:for-each select="//variables/item">
<xsl:if test="normalize-space(@public)!=''">
<xsl:call-template name="init-variables"/>
</xsl:if>
<xsl:if test="normalize-space(@private)=''">
<xsl:if test="normalize-space(@public)=''">
<xsl:call-template name="init-variables"/>
</xsl:if>
</xsl:if>
</xsl:for-each>
sleep_msec(<xsl:call-template name="settings"><xsl:with-param name="varname" select="'sleep-msec'"/></xsl:call-template>),
active(true),
<xsl:if test="normalize-space($TESTMODE)!=''">
isTestMode(false),
idTestMode_S(uniset_conf()->getSensorID("TestMode_S")),
idLocalTestMode_S(uniset_conf()->getSensorID(uniset_conf()->getProp(cnode,"LocalTestMode_S"))),
in_TestMode_S(false),
in_LocalTestMode_S(false),
</xsl:if>
idHeartBeat(DefaultObjectId),
maxHeartBeat(10),
confnode(cnode),
smReadyTimeout(0),
activated(false),
askPause(uniset_conf()->getPIntProp(cnode,"askPause",2000)),
forceOut(false),
<xsl:for-each select="//variables/item">
<xsl:if test="normalize-space(@private)!=''">
<xsl:call-template name="init-variables"/>
</xsl:if>
</xsl:for-each>
end_private(false)
{
	auto conf = uniset_conf();

	<xsl:call-template name="COMMON-ID-LIST"/>

	if( getId() == DefaultObjectId )
	{
		ostringstream err;
		err &lt;&lt; "(<xsl:value-of select="$CLASSNAME"/>::init): Unknown ObjectID!";
		throw SystemError( err.str() );
	}

	mylog.setLogName(myname);

<xsl:for-each select="//smap/item">
	<xsl:if test="normalize-space(@no_check_id)!='1'">
	if( <xsl:value-of select="normalize-space(@name)"/> == UniSetTypes::DefaultObjectId )
		throw Exception( myname + ": Not found ID for (<xsl:value-of select="@name"/>) " + conf->getProp(cnode,"<xsl:value-of select="@name"/>") );
	
	</xsl:if>
	
	if( node_<xsl:value-of select="normalize-space(@name)"/> == UniSetTypes::DefaultObjectId )
	{
		<xsl:if test="normalize-space(@no_check_id)!='1'">
		if( !conf->getProp(cnode,"node_<xsl:value-of select="normalize-space(@name)"/>").empty() )
			throw Exception( myname + ": Not found NodeID for (node='node_<xsl:value-of select="normalize-space(@name)"/>') " + conf->getProp(cnode,"node_<xsl:value-of select="normalize-space(@name)"/>") );
		</xsl:if>
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

	UniXML::iterator it(cnode);

	forceOut = conf->getArgPInt("--" + argprefix + "force-out",it.getProp("forceOut"),false);

	string heart = conf->getArgParam("--" + argprefix + "heartbeat-id",it.getProp("heartbeat_id"));
	if( !heart.empty() )
	{
		idHeartBeat = conf->getSensorID(heart);
		if( idHeartBeat == DefaultObjectId )
		{
			ostringstream err;
			err &lt;&lt; myname &lt;&lt; ": не найден идентификатор для датчика 'HeartBeat' " &lt;&lt; heart;
			throw SystemError(err.str());
		}

		int heartbeatTime = conf->getArgPInt("--" + argprefix + "heartbeat-time",it.getProp("heartbeatTime"),conf-&gt;getHeartBeatTime());
		if( heartbeatTime>0 )
			ptHeartBeat.setTiming(heartbeatTime);
		else
			ptHeartBeat.setTiming(UniSetTimer::WaitUpTime);

		maxHeartBeat = conf->getArgPInt("--" + argprefix + "heartbeat-max",it.getProp("heartbeat_max"), 10);
	}

	// Инициализация значений
	<xsl:for-each select="//smap/item">
		<xsl:if test="normalize-space(@default)=''">
			<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = conf->getArgPInt("--" + argprefix + "<xsl:value-of select="@name"/>-default",it.getProp("<xsl:value-of select="@name"/>_default"),0);
		</xsl:if>
		<xsl:if test="not(normalize-space(@default)='')">
			<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = conf->getArgPInt("--" + argprefix + "<xsl:value-of select="@name"/>-default",it.getProp("<xsl:value-of select="@name"/>_default"),<xsl:value-of select="@default"/>);
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
	
	sleep_msec = conf->getArgPInt("--" + argprefix + "sleep-msec","<xsl:call-template name="settings"><xsl:with-param name="varname" select="'sleep-msec'"/></xsl:call-template>", <xsl:call-template name="settings"><xsl:with-param name="varname" select="'sleep-msec'"/></xsl:call-template>);

	resetMsgTime = conf->getPIntProp(cnode,"resetMsgTime", 2000);
	ptResetMsg.setTiming(resetMsgTime);

	smReadyTimeout = conf->getArgInt("--" + argprefix + "sm-ready-timeout","<xsl:call-template name="settings"><xsl:with-param name="varname" select="'smReadyTimeout'"/></xsl:call-template>");
	if( smReadyTimeout == 0 )
		smReadyTimeout = 60000;
	else if( smReadyTimeout &lt; 0 )
		smReadyTimeout = UniSetTimer::WaitUpTime;

	smTestID = conf->getSensorID(init3_str(conf->getArgParam("--" + argprefix + "sm-test-id"),conf->getProp(cnode,"smTestID"),""));
	<xsl:for-each select="//smap/item">
	<xsl:if test="normalize-space(@smTestID)!=''">
	if( smTestID == DefaultObjectId )
		smTestID = <xsl:value-of select="@name"/>;
	</xsl:if>
	</xsl:for-each>

	activateTimeout	= conf->getArgPInt("--" + argprefix + "activate-timeout", 20000);

	int msec = conf->getArgPInt("--" + argprefix + "startup-timeout", 10000);
	ptStartUpTimeout.setTiming(msec);

	// ===================== &lt;variables&gt; =====================
	<xsl:for-each select="//variables/item">
	<xsl:if test="normalize-space(@min)!=''">
	if( <xsl:value-of select="@name"/> &lt; <xsl:value-of select="@min"/> )
	{
        uwarn &lt;&lt; myname &lt;&lt; ": RANGE WARNING: <xsl:value-of select="@name"/>=" &lt;&lt; <xsl:value-of select="@name"/> &lt;&lt; " &lt; <xsl:value-of select="@min"/>" &lt;&lt; endl;
		<xsl:if test="normalize-space(@no_range_exception)=''">throw UniSetTypes::SystemError(myname+"(init): <xsl:value-of select="@name"/> &lt; <xsl:value-of select="@min"/>");</xsl:if>
	}
	</xsl:if>
	<xsl:if test="normalize-space(@max)!=''">
	if( <xsl:value-of select="@name"/> &gt; <xsl:value-of select="@max"/> )
	{
        uwarn &lt;&lt; myname &lt;&lt; ": RANGE WARNING: <xsl:value-of select="@name"/>=" &lt;&lt; <xsl:value-of select="@name"/> &lt;&lt; " &gt; <xsl:value-of select="@max"/>" &lt;&lt; endl;
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
<xsl:if test="normalize-space($TESTMODE)!=''">
	in_TestMode_S  		= (idTestMode_S!=DefaultObjectId) ? ui->getValue(idTestMode_S):false;
	in_LocalTestMode_S 	= (idLocalTestMode_S!=DefaultObjectId) ? ui->getValue(idLocalTestMode_S):false;
</xsl:if>
		<xsl:for-each select="//smap/item">
			<xsl:choose>
				<xsl:when test="normalize-space(@vartype)='in'"><xsl:call-template name="getdata"/></xsl:when>
			</xsl:choose>
		</xsl:for-each>
<!--
	<xsl:for-each select="//smap/item">
		<xsl:choose>
				<xsl:when test="normalize-space(@vartype)='in'"><xsl:call-template name="getdata"><xsl:with-param name="output" select="1"/></xsl:call-template></xsl:when>
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
		</xsl:choose>
	</xsl:for-each>
}
// -----------------------------------------------------------------------------
bool <xsl:value-of select="$CLASSNAME"/>_SK::alarm( UniSetTypes::ObjectId _code, bool _state )
{
	if( _code == UniSetTypes::DefaultObjectId )
	{
        ucrit  &lt;&lt; getName()
				&lt;&lt; "(alarm): попытка послать сообщение с DefaultObjectId"
				&lt;&lt; endl;
		return false;	
	}

    ulog1 &lt;&lt; getName()  &lt;&lt; "(alarm): " &lt;&lt; ( _state ? "SEND " : "RESET " ) &lt;&lt; endl;
	
	<xsl:for-each select="//msgmap/item">
	if( _code == <xsl:value-of select="@name"/> )
	{
        ulog1 &lt;&lt; "<xsl:value-of select="@name"/>" &lt;&lt; endl;
		try
		{
			m_<xsl:value-of select="@name"/> = _state;
			// сохраняем сразу...
			si.id = <xsl:value-of select="@name"/>;
			si.node = node_<xsl:value-of select="@name"/>;
			ui->setValue( si,m_<xsl:value-of select="@name"/>,getId() );
			return true;
		}
	    catch( std::exception&amp;ex )
    	{
        	ucrit &lt;&lt; myname &lt;&lt; "(execute): catch " &lt;&lt; ex.what()  &lt;&lt;   endl;
	    }

		return false;
	}
	</xsl:for-each>
	
    ulog1 &lt;&lt; " not found MessgeOID?!!" &lt;&lt; endl;
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
			ui->setValue( si, 0, getId() );
		}
		catch( UniSetTypes::Exception&amp; ex )
		{
            ulog1 &lt;&lt; getName() &lt;&lt; ex &lt;&lt; endl;
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
	</xsl:choose>
	</xsl:for-each>
}
// -----------------------------------------------------------------------------
</xsl:template>


<xsl:template name="COMMON-CC-ALONE-FUNCS">
// -----------------------------------------------------------------------------
// ( val, confval, default val )
static const std::string init3_str( const std::string&amp; s1, const std::string&amp; s2, const std::string&amp; s3 )
{
	if( !s1.empty() )
		return s1;
	if( !s2.empty() )
		return s2;

	return s3;
}
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
<xsl:if test="normalize-space($TESTMODE)!=''">
isTestMode(false),
idTestMode_S(DefaultObjectId),
idLocalTestMode_S(DefaultObjectId),
</xsl:if>
idHeartBeat(DefaultObjectId),
maxHeartBeat(10),
confnode(0),
activated(false),
askPause(2000),
forceOut(false)
{
	ucrit &lt;&lt; "<xsl:value-of select="$CLASSNAME"/>: init failed!!!!!!!!!!!!!!!" &lt;&lt; endl;
	throw Exception( string(myname+": init failed!!!") );
}
// -----------------------------------------------------------------------------
<xsl:value-of select="$CLASSNAME"/>_SK::<xsl:value-of select="$CLASSNAME"/>_SK( ObjectId id, xmlNode* cnode, const string&amp; argprefix ):
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
<xsl:if test="normalize-space($TESTMODE)!=''">
isTestMode(false),
idTestMode_S(uniset_conf()->getSensorID("TestMode_S")),
idLocalTestMode_S(uniset_conf()->getSensorID(uniset_conf()->getProp(cnode,"LocalTestMode_S"))),
in_TestMode_S(false),
in_LocalTestMode_S(false),
</xsl:if>
idHeartBeat(DefaultObjectId),
maxHeartBeat(10),
confnode(cnode),
activated(false),
askPause(uniset_conf()->getPIntProp(cnode,"askPause",2000))
{
	auto conf = uniset_conf();

	if( getId() == DefaultObjectId )
	{
		ostringstream err;
		err &lt;&lt; "(<xsl:value-of select="$CLASSNAME"/>::init): Unknown ObjectID!";
		throw SystemError( err.str() );
	}

	mylog.setLogName(myname);
	
	si.node = conf->getLocalNode();


<xsl:for-each select="//sensors/item">
	<xsl:call-template name="setmsg">
		<xsl:with-param name="GENTYPE" select="'CHECK'"/>
	</xsl:call-template>
</xsl:for-each>

	UniXML::iterator it(cnode);

    forceOut = conf->getArgPInt("--" + argprefix + "force-out",it.getProp("forceOut"),false);

	string heart = conf->getArgParam("--" + argprefix + "heartbeat-id",it.getProp("heartbeat_id"));
	if( !heart.empty() )
	{
		idHeartBeat = conf->getSensorID(heart);
		if( idHeartBeat == DefaultObjectId )
		{
			ostringstream err;
			err &lt;&lt; myname &lt;&lt; ": не найден идентификатор для датчика 'HeartBeat' " &lt;&lt; heart;
			throw SystemError(err.str());
		}

		int heartbeatTime = conf->getArgPInt("--" + argprefix + "heartbeat-time",it.getProp("heartbeatTime"),conf-&gt;getHeartBeatTime());

		if( heartbeatTime>0 )
			ptHeartBeat.setTiming(heartbeatTime);
		else
			ptHeartBeat.setTiming(UniSetTimer::WaitUpTime);

		maxHeartBeat = conf->getArgPInt("--" + argprefix + "heartbeat-max",it.getProp("heartbeat_max"), 10);
	}


	sleep_msec = conf->getArgPInt("--" + argprefix + "sleep-msec","<xsl:call-template name="settings-alone"><xsl:with-param name="varname" select="'sleep-msec'"/></xsl:call-template>", <xsl:call-template name="settings-alone"><xsl:with-param name="varname" select="'sleep-msec'"/></xsl:call-template>);

	resetMsgTime = conf->getPIntProp(cnode,"resetMsgTime", 2000);
	ptResetMsg.setTiming(resetMsgTime);

	smReadyTimeout = conf->getArgInt("--" + argprefix + "sm-ready-timeout","<xsl:call-template name="settings"><xsl:with-param name="varname" select="'smReadyTimeout'"/></xsl:call-template>");
	if( smReadyTimeout == 0 )
		smReadyTimeout = 60000;
	else if( smReadyTimeout &lt; 0 )
		smReadyTimeout = UniSetTimer::WaitUpTime;

	smTestID = conf->getSensorID(init3_str(conf->getArgParam("--" + argprefix + "sm-test-id"),conf->getProp(cnode,"smTestID"),""));

	activateTimeout	= conf->getArgPInt("--" + argprefix + "activate-timeout", 20000);

	int msec = conf->getArgPInt("--" + argprefix + "startup-timeout", 10000);
	ptStartUpTimeout.setTiming(msec);
}

// -----------------------------------------------------------------------------

<xsl:value-of select="$CLASSNAME"/>_SK::~<xsl:value-of select="$CLASSNAME"/>_SK()
{
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::updateValues()
{
<xsl:if test="normalize-space($TESTMODE)!=''">
	in_TestMode_S  		= (idTestMode_S!=DefaultObjectId) ? ui->getValue(idTestMode_S):false;
	in_LocalTestMode_S 	= (idLocalTestMode_S!=DefaultObjectId) ? ui->getValue(idLocalTestMode_S):false;
</xsl:if>
	// Опрашиваем все входы...
	<xsl:for-each select="//sensors/item/consumers/consumer">
		<xsl:choose>
			<xsl:when test="normalize-space(@vartype)='in'"><xsl:call-template name="getdata"/></xsl:when>
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
        ucrit  &lt;&lt; getName()
				&lt;&lt; "(alarm): попытка послать сообщение с DefaultObjectId"
				&lt;&lt; endl;
		return false;	
	}

    ulog1 &lt;&lt; getName()  &lt;&lt; "(alarm): (" &lt;&lt; _code  &lt;&lt; ")"  &lt;&lt; ( _state ? "SEND" : "RESET" ) &lt;&lt; endl;

<xsl:for-each select="//sensors/item">
	<xsl:call-template name="setmsg">
		<xsl:with-param name="GENTYPE" select="'A'"/>
	</xsl:call-template>
</xsl:for-each>
	
    ulog8 &lt;&lt; " not found MessgeOID?!!" &lt;&lt; endl;
	return false;
}
// -----------------------------------------------------------------------------
</xsl:template>

<xsl:template name="check_changes">
<xsl:param name="onlymsg"></xsl:param>
        <xsl:if test="normalize-space($onlymsg)=''">
        if( prev_<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> != <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> )
        </xsl:if>
        {
        <xsl:if test="normalize-space($onlymsg)=''">
        </xsl:if>
          // приходится искуственно использовать третий параметр,
          // что-бы компилятор выбрал
          // правильный(для аналоговых) конструктор у SensorMessage
            IOController_i::CalibrateInfo _ci;
            SensorMessage _sm( <xsl:value-of select="@name"/>, (long)<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>, _ci);
            _sm.sensor_type = UniversalIO::AI;
            sensorInfo(&amp;_sm);
        }
</xsl:template>

</xsl:stylesheet>
