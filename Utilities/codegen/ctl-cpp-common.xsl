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
                mylog8 &lt;&lt; "<xsl:value-of select="../../@name"/>" &lt;&lt; endl;
				m_<xsl:value-of select="../../@name"/> = _state;
				try
				{
					// сохраняем сразу...
					si.id = mid_<xsl:value-of select="../../@name"/>;
					ui->setValue( si,(m_<xsl:value-of select="../../@name"/> ? 1:0), getId() );
					return true;
				}
			    catch( const std::exception&amp;ex )
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
					catch( const UniSetTypes::Exception&amp; ex )
					{
                        mywarn &lt;&lt; getName() &lt;&lt; ex &lt;&lt; endl;
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
		long getValue( UniSetTypes::ObjectId sid );
		void setValue( UniSetTypes::ObjectId sid, long value );
		void askSensor( UniSetTypes::ObjectId sid, UniversalIO::UIOCommand, UniSetTypes::ObjectId node = UniSetTypes::uniset_conf()->getLocalNode() );
		void updateValues();

		virtual UniSetTypes::SimpleInfo* getInfo() override;

		virtual bool setMsg( UniSetTypes::ObjectId code, bool state = true );

		inline std::shared_ptr&lt;DebugStream&gt; log(){ return mylog; }
		inline std::shared_ptr&lt;LogAgregator&gt; logAgregator(){ return loga; }

		void init_dlog( std::shared_ptr&lt;DebugStream&gt; d );

        // "синтаксический сахар"..для логов
        #ifndef myinfo 
        	#define myinfo if( log()->debugging(Debug::INFO) ) log()->info() 
        #endif
        #ifndef mywarn
	        #define mywarn if( log()->debugging(Debug::WARN) ) log()->warn()
        #endif
        #ifndef mycrit
    	    #define mycrit if( log()->debugging(Debug::CRIT) ) log()->crit()
        #endif
        #ifndef mylog1
        	#define mylog1 if( log()->debugging(Debug::LEVEL1) ) log()->level1()
        #endif
        #ifndef mylog2
	        #define mylog2 if( log()->debugging(Debug::LEVEL2) ) log()->level2()
        #endif
        #ifndef mylog3
    	    #define mylog3 if( log()->debugging(Debug::LEVEL3) ) log()->level3()
        #endif
        #ifndef mylog4
        	#define mylog4 if( log()->debugging(Debug::LEVEL4) ) log()->level4()
        #endif
        #ifndef mylog5
	        #define mylog5 if( log()->debugging(Debug::LEVEL5) ) log()->level5()
        #endif
        #ifndef mylog6
    	    #define mylog6 if( log()->debugging(Debug::LEVEL6) ) log()->level6()
        #endif
        #ifndef mylog7
        	#define mylog7 if( log()->debugging(Debug::LEVEL7) ) log()->level7()
        #endif
        #ifndef mylog8
	        #define mylog8 if( log()->debugging(Debug::LEVEL8) ) log()->level8()
        #endif
        #ifndef mylog9
    	    #define mylog9 if( log()->debugging(Debug::LEVEL9) ) log()->level9()
        #endif
        #ifndef mylogany
        	#define mylogany log()->any()
        #endif
        
        // Вспомогательные функции для удобства логирования
        // ------------------------------------------------------------
        /*!&lt; вывод в строку значение всех входов и выходов в формате 
           ObjectName: 
              in_xxx  = val
              in_xxx2 = val
              out_zzz = val
              ...
        */
        std::string dumpIO();
        
        /*!&lt; Вывод в строку названия входа/выхода в формате: in_xxx(SensorName) 
           \param id           - идентификатор датчика
           \param showLinkName - TRUE - выводить SensorName, FALSE - не выводить
        */
        std::string str( UniSetTypes::ObjectId id, bool showLinkName=true );
        
        /*!&lt; Вывод значения входа/выхода в формате: in_xxx(SensorName)=val 
           \param id           - идентификатор датчика
           \param showLinkName - TRUE - выводить SensorName, FALSE - не выводить
        */
        std::string strval( UniSetTypes::ObjectId id, bool showLinkName=true );        
        // ------------------------------------------------------------
        
</xsl:template>

<xsl:template name="COMMON-HEAD-PROTECTED">
		virtual void callback() override;
		virtual void processingMessage( UniSetTypes::VoidMessage* msg ) override;
		virtual void sysCommand( const UniSetTypes::SystemMessage* sm ){};
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
		void preSysCommand( const UniSetTypes::SystemMessage* sm );
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
		const std::string argprefix;
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
		
		std::shared_ptr&lt;LogAgregator&gt; loga;
		std::shared_ptr&lt;DebugStream&gt; mylog;
		std::shared_ptr&lt;LogServer&gt; logserv;
		std::string logserv_host = {""};
		int logserv_port = {0};
</xsl:template>

<xsl:template name="COMMON-HEAD-PRIVATE">

</xsl:template>

<xsl:template name="COMMON-CC-FILE">
// ------------------------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::init_dlog( std::shared_ptr&lt;DebugStream&gt; d )
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
				preSysCommand( reinterpret_cast&lt;SystemMessage*&gt;(_msg) );
			break;

			default:
				break;
		}	
	}
	catch( const Exception&amp; ex )
	{
		ucrit  &lt;&lt; myname &lt;&lt; "(processingMessage): " &lt;&lt; ex &lt;&lt; endl;
	}
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::preSysCommand( const SystemMessage* _sm )
{
	switch( _sm->command )
	{
		case SystemMessage::WatchDog:
			uinfo &lt;&lt; myname &lt;&lt; "(preSysCommand): WatchDog" &lt;&lt; endl;
			if( !active || !ptStartUpTimeout.checkTime() )
			{
				uwarn &lt;&lt; myname &lt;&lt; "(preSysCommand): игнорируем WatchDog, потому-что только-что стартанули" &lt;&lt; endl;
				break;
			}
		case SystemMessage::StartUp:
		{
			if( !logserv_host.empty() &amp;&amp; logserv_port != 0 &amp;&amp; !logserv-&gt;isRunning() )
			{
				myinfo &lt;&lt; myname &lt;&lt; "(preSysCommand): run log server " &lt;&lt; logserv_host &lt;&lt; ":" &lt;&lt; logserv_port &lt;&lt; endl;
				logserv-&gt;run(logserv_host, logserv_port, true);
			}

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
			mylogany &lt;&lt; myname &lt;&lt; "(preSysCommand): logRotate" &lt;&lt; endl;
			string fname( log()->getLogFile() );
			if( !fname.empty() )
			{
				mylog->logFile(fname.c_str(),true);
				mylogany &lt;&lt; myname &lt;&lt; "(preSysCommand): ***************** mylog LOG ROTATE *****************" &lt;&lt; endl;
			}
		}
		break;

		default:
			break;
	}
	
	sysCommand(_sm);
}
// -----------------------------------------------------------------------------
UniSetTypes::SimpleInfo* <xsl:value-of select="$CLASSNAME"/>_SK::getInfo()
{
	<xsl:if test="not(normalize-space($BASECLASS)='')">UniSetTypes::SimpleInfo_var i = <xsl:value-of select="$BASECLASS"/>::getInfo();</xsl:if>
	<xsl:if test="normalize-space($BASECLASS)=''">UniSetTypes::SimpleInfo_var i = UniSetObject::getInfo();</xsl:if>
	
	ostringstream inf;
	
	inf &lt;&lt; i->info &lt;&lt; endl;
	inf &lt;&lt; dumpIO() &lt;&lt; endl;
	
	i->info =  inf.str().c_str();
	
	return i._retn();
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
	// см. preSysCommand()
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
<xsl:if test="normalize-space(@type)='sensor'"><xsl:value-of select="normalize-space(@name)"/>(DefaultObjectId),</xsl:if>
<xsl:if test="normalize-space(@type)='object'"><xsl:value-of select="normalize-space(@name)"/>(DefaultObjectId),</xsl:if>
</xsl:if>
</xsl:template>
<xsl:template name="init-variables">
<xsl:if test="normalize-space(@type)='int'">
<xsl:value-of select="normalize-space(@name)"/>(uni_atoi( init3_str(uniset_conf()->getArgParam("--" + (_argprefix.empty() ? myname+"-" : _argprefix) + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>"))),
</xsl:if>
<xsl:if test="normalize-space(@type)='long'">
<xsl:value-of select="normalize-space(@name)"/>(uni_atoi( init3_str(uniset_conf()->getArgParam("--" + (_argprefix.empty() ? myname+"-" : _argprefix) + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>"))),
</xsl:if>
<xsl:if test="normalize-space(@type)='float'">
<xsl:value-of select="normalize-space(@name)"/>(atof( init3_str(uniset_conf()->getArgParam("--" + (_argprefix.empty() ? myname+"-" : _argprefix) + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>").c_str())),
</xsl:if>
<xsl:if test="normalize-space(@type)='double'">
<xsl:value-of select="normalize-space(@name)"/>(atof( init3_str(uniset_conf()->getArgParam("--" + (_argprefix.empty() ? myname+"-" : _argprefix) + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>").c_str())),
</xsl:if>
<xsl:if test="normalize-space(@type)='bool'">
<xsl:value-of select="normalize-space(@name)"/>(uni_atoi( init3_str(uniset_conf()->getArgParam("--" + (_argprefix.empty() ? myname+"-" : _argprefix) + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>"))),
</xsl:if>
<xsl:if test="normalize-space(@type)='str'">
<xsl:value-of select="normalize-space(@name)"/>(init3_str(uniset_conf()->getArgParam("--" + (_argprefix.empty() ? myname+"-" : _argprefix) + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>")),
</xsl:if>
<xsl:if test="normalize-space(@type)='sensor'">
<xsl:value-of select="normalize-space(@name)"/>(uniset_conf()->getSensorID(init3_str(uniset_conf()->getArgParam("--" + (_argprefix.empty() ? myname+"-" : _argprefix) + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>"))),
</xsl:if>
<xsl:if test="normalize-space(@type)='object'">
<xsl:value-of select="normalize-space(@name)"/>(uniset_conf()->getObjectID(init3_str(uniset_conf()->getArgParam("--" + (_argprefix.empty() ? myname+"-" : _argprefix) + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>"))),
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
#include &lt;memory&gt;
#include &lt;iomanip&gt;
#include <xsl:call-template name="preinclude"/>Configuration.h<xsl:call-template name="postinclude"/>
#include <xsl:call-template name="preinclude"/>Exceptions.h<xsl:call-template name="postinclude"/>
#include <xsl:call-template name="preinclude"/>ORepHelpers.h<xsl:call-template name="postinclude"/>
#include <xsl:call-template name="preinclude"/>LogServer.h<xsl:call-template name="postinclude"/>
#include <xsl:call-template name="preinclude"/>DebugStream.h<xsl:call-template name="postinclude"/>
#include <xsl:call-template name="preinclude"/>LogAgregator.h<xsl:call-template name="postinclude"/>
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
<xsl:value-of select="$CLASSNAME"/>_SK::<xsl:value-of select="$CLASSNAME"/>_SK( ObjectId id, xmlNode* cnode, const std::string&amp; _argprefix ):
<xsl:if test="normalize-space($BASECLASS)!=''"><xsl:value-of select="normalize-space($BASECLASS)"/>(id),</xsl:if>
<xsl:if test="normalize-space($BASECLASS)=''">UniSetObject(id),</xsl:if>
argprefix( (_argprefix.empty() ? myname+"-" : _argprefix) ),
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

    mylog = make_shared&lt;DebugStream&gt;();
	mylog-&gt;setLogName(myname);
	{
		ostringstream s;
		s &lt;&lt; argprefix &lt;&lt; "log";
		conf->initLogStream(mylog,s.str());
	}

	loga = make_shared&lt;LogAgregator&gt;(myname+"-loga");
	loga-&gt;add(mylog);
	loga-&gt;add(ulog());

	logserv = make_shared&lt;LogServer&gt;(loga);
	logserv-&gt;init( argprefix + "logserver", confnode );

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

	// ------- init logserver ---
	if( findArgParam("--" + argprefix + "run-logserver", conf-&gt;getArgc(), conf-&gt;getArgv()) != -1 )
	{
		logserv_host = conf-&gt;getArg2Param("--" + argprefix + "logserver-host", it.getProp("logserverHost"), "localhost");
		logserv_port = conf-&gt;getArgPInt("--" + argprefix + "logserver-port", it.getProp("logserverPort"), getId());
	}
	
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

	string s_resetTime("<xsl:call-template name="settings"><xsl:with-param name="varname" select="'resetMsgTime'"/></xsl:call-template>");
	if( s_resetTime.empty() )
		s_resetTime = "500";

	resetMsgTime = uni_atoi(init3_str(conf->getArgParam("--" + argprefix + "resetMsgTime"),conf->getProp(cnode,"resetMsgTime"),s_resetTime));
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
bool <xsl:value-of select="$CLASSNAME"/>_SK::setMsg( UniSetTypes::ObjectId _code, bool _state )
{
	if( _code == UniSetTypes::DefaultObjectId )
	{
		mycrit &lt;&lt; myname &lt;&lt; "(setMsg): попытка послать сообщение с DefaultObjectId" &lt;&lt; endl;
		return false;	
	}

    mylog8 &lt;&lt; myname &lt;&lt; "(setMsg): " &lt;&lt; ( _state ? "SEND " : "RESET " ) &lt;&lt; endl;

    // взводим автоматический сброс
    if( _state )
        ptResetMsg.reset();

	<xsl:for-each select="//msgmap/item">
	if( _code == <xsl:value-of select="@name"/> )
	{
        mylog8 &lt;&lt; "<xsl:value-of select="@name"/>" &lt;&lt; endl;
		try
		{
			m_<xsl:value-of select="@name"/> = _state;
			// сохраняем сразу...
			si.id = <xsl:value-of select="@name"/>;
			si.node = node_<xsl:value-of select="@name"/>;
			ui->setValue( si,m_<xsl:value-of select="@name"/>,getId() );
			return true;
		}
	    catch( const std::exception&amp;ex )
    	{
        	mycrit &lt;&lt; myname &lt;&lt; "(setMsg): catch " &lt;&lt; ex.what()  &lt;&lt;   endl;
	    }

		return false;
	}
	</xsl:for-each>
	
    mylog8 &lt;&lt; myname &lt;&lt; "(setMsg): not found MessgeOID?!!" &lt;&lt; endl;
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
		catch( const UniSetTypes::Exception&amp; ex )
		{
            mywarn &lt;&lt; getName() &lt;&lt; ex &lt;&lt; endl;
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
std::string  <xsl:value-of select="$CLASSNAME"/>_SK::dumpIO()
{
	ostringstream s;
	s &lt;&lt; myname &lt;&lt; ": " &lt;&lt; endl;
	<xsl:for-each select="//smap/item">
		s &lt;&lt; "    " &lt;&lt; setw(20) &lt;&lt; std::right &lt;&lt; "<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>"
				&lt;&lt; "(" &lt;&lt; setw(20) &lt;&lt; std::left &lt;&lt; ORepHelpers::getShortName( uniset_conf()->oind->getMapName(<xsl:value-of select="@name"/>)) &lt;&lt; ")"
				&lt;&lt; std::right &lt;&lt; " = " &lt;&lt; setw(6) &lt;&lt; <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>
				&lt;&lt; endl;
	</xsl:for-each>
	return s.str();
}
// ----------------------------------------------------------------------------
std::string  <xsl:value-of select="$CLASSNAME"/>_SK::str( UniSetTypes::ObjectId id, bool showLinkName )
{
	ostringstream s;
	<xsl:for-each select="//smap/item">
	if( id == <xsl:value-of select="@name"/> )
	{
		s &lt;&lt; "<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>";
		if( showLinkName ) s &lt;&lt; "(" &lt;&lt; ORepHelpers::getShortName( uniset_conf()->oind->getMapName(<xsl:value-of select="@name"/>)) &lt;&lt; ")";
		return s.str();
	}
	</xsl:for-each>	
	return "";
}
// ----------------------------------------------------------------------------
std::string  <xsl:value-of select="$CLASSNAME"/>_SK::strval( UniSetTypes::ObjectId id, bool showLinkName )
{
	ostringstream s;
	<xsl:for-each select="//smap/item">
	if( id == <xsl:value-of select="@name"/> )
	{
		// s &lt;&lt; str(id,showLinkName) &lt;&lt; "=" &lt;&lt; <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>;
		s &lt;&lt; "<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>";
		if( showLinkName ) s &lt;&lt; "(" &lt;&lt; ORepHelpers::getShortName( uniset_conf()->oind->getMapName(<xsl:value-of select="@name"/>)) &lt;&lt; ")";		
		s &lt;&lt; "=" &lt;&lt; <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>;
		return s.str();
	}
	</xsl:for-each>	
	return "";
}
// ----------------------------------------------------------------------------
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
	mycrit &lt;&lt; "<xsl:value-of select="$CLASSNAME"/>: init failed!!!!!!!!!!!!!!!" &lt;&lt; endl;
	throw Exception( string(myname+": init failed!!!") );
}
// -----------------------------------------------------------------------------
<xsl:value-of select="$CLASSNAME"/>_SK::<xsl:value-of select="$CLASSNAME"/>_SK( ObjectId id, xmlNode* cnode, const string&amp; _argprefix ):
<xsl:if test="normalize-space($BASECLASS)!=''"><xsl:value-of select="normalize-space($BASECLASS)"/>(id),</xsl:if>
<xsl:if test="normalize-space($BASECLASS)=''">UniSetObject(id),</xsl:if>
argprefix( (_argprefix.empty() ? myname+"-" : _argprefix) ),
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

	mylog = make_shared&lt;DebugStream&gt;();
	mylog-&gt;setLogName(myname);
    {
        ostringstream s;
        s &lt;&lt; argprefix &lt;&lt; "log";
        conf->initLogStream(mylog, s.str());
    }

	loga = make_shared&lt;LogAgregator&gt;(myname+"-loga");
	loga-&gt;add(mylog);
	loga-&gt;add(ulog());

	logserv = make_shared&lt;LogServer&gt;(loga);
	logserv-&gt;init( argprefix + "logserver", confnode );
	
	si.node = conf->getLocalNode();

<xsl:for-each select="//sensors/item">
	<xsl:call-template name="setmsg">
		<xsl:with-param name="GENTYPE" select="'CHECK'"/>
	</xsl:call-template>
</xsl:for-each>

	UniXML::iterator it(cnode);

	// ------- init logserver ---
	if( findArgParam("--" + argprefix + "run-logserver", conf-&gt;getArgc(), conf-&gt;getArgv()) != -1 )
	{
		logserv_host = conf-&gt;getArg2Param("--" + argprefix + "logserver-host", it.getProp("logserverHost"), "localhost");
		logserv_port = conf-&gt;getArgPInt("--" + argprefix + "logserver-port", it.getProp("logserverPort"), getId());
	}
	
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

	string s_resetTime("<xsl:call-template name="settings-alone"><xsl:with-param name="varname" select="'sleep-msec'"/></xsl:call-template>");
	if( s_resetTime.empty() )
		s_resetTime = "500";

	resetMsgTime = uni_atoi(init3_str(conf->getArgParam("--" + argprefix + "resetMsgTime"),conf->getProp(cnode,"resetMsgTime"),s_resetTime));
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
bool <xsl:value-of select="$CLASSNAME"/>_SK::setMsg( UniSetTypes::ObjectId _code, bool _state )
{
	if( _code == UniSetTypes::DefaultObjectId )
	{
        mycrit  &lt;&lt; myname &lt;&lt; "(setMsg): попытка послать сообщение с DefaultObjectId" &lt;&lt; endl;
		return false;	
	}

    mylog8 &lt;&lt; myname &lt;&lt; "(setMsg): (" &lt;&lt; _code  &lt;&lt; ")"  &lt;&lt; ( _state ? "SEND" : "RESET" ) &lt;&lt; endl;

    // взводим таймер автоматического сброса
    if( _state )
        ptResetMsg.reset();
    
<xsl:for-each select="//sensors/item">
	<xsl:call-template name="setmsg">
		<xsl:with-param name="GENTYPE" select="'A'"/>
	</xsl:call-template>
</xsl:for-each>
	
    mylog8 &lt;&lt; myname &lt;&lt; "(setMsg): not found MessgeOID?!!" &lt;&lt; endl;
	return false;
}
// -----------------------------------------------------------------------------
std::string  <xsl:value-of select="$CLASSNAME"/>_SK::dumpIO()
{
	ostringstream s;
	s &lt;&lt; myname &lt;&lt; ": " &lt;&lt; endl;
	
	<xsl:for-each select="//sensors/item/consumers/consumer">
	<xsl:if test="normalize-space(../../@msg)!='1'">
	<xsl:if test="normalize-space(@name)=$OID">
	s &lt;&lt; "   " &lt;&lt; strval(<xsl:value-of select="../../@name"/>) &lt;&lt; endl;
	</xsl:if>
	</xsl:if>
	</xsl:for-each>
	return s.str();
}
// ----------------------------------------------------------------------------
std::string  <xsl:value-of select="$CLASSNAME"/>_SK::str( UniSetTypes::ObjectId id, bool showLinkName )
{
	ostringstream s;
	<xsl:for-each select="//sensors/item/consumers/consumer">
	<xsl:if test="normalize-space(../../@msg)!='1'">
	<xsl:if test="normalize-space(@name)=$OID">
	if( id == <xsl:value-of select="../../@name"/> )
	{
		s &lt;&lt; "<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>";
		if( showLinkName ) s &lt;&lt; "(<xsl:value-of select="../../@name"/>)";
		return s.str();
	}
	</xsl:if>
	</xsl:if>
	</xsl:for-each>
	return "";
}
// ----------------------------------------------------------------------------
std::string  <xsl:value-of select="$CLASSNAME"/>_SK::strval( UniSetTypes::ObjectId id, bool showLinkName )
{
	ostringstream s;
	<xsl:for-each select="//sensors/item/consumers/consumer">
	<xsl:if test="normalize-space(../../@msg)!='1'">
	<xsl:if test="normalize-space(@name)=$OID">
	if( id == <xsl:value-of select="../../@name"/> )
	{
		s &lt;&lt; "<xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>";
		if( showLinkName ) s &lt;&lt; "(<xsl:value-of select="../../@name"/>)";
		s &lt;&lt; "=" &lt;&lt; <xsl:call-template name="setprefix"/><xsl:value-of select="../../@name"/>;
		return s.str();
	}
	</xsl:if>
	</xsl:if>
	</xsl:for-each>
	return "";
}
// ----------------------------------------------------------------------------
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
