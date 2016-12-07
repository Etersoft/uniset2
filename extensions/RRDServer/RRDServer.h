/*
 * Copyright (c) 2015 Pavel Vainerman.
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
// -----------------------------------------------------------------------------
#ifndef _RRDServer_H_
#define _RRDServer_H_
// -----------------------------------------------------------------------------
#include <unordered_map>
#include <list>
#include <memory>
#include "UObject_SK.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "extensions/Extensions.h"
// --------------------------------------------------------------------------
namespace uniset
{
// -----------------------------------------------------------------------------
/*!
    \page page_RRDServer Реализация RRD хранилища

      - \ref sec_RRD_Comm
      - \ref sec_RRD_Conf
	  - \ref sec_RRD_DSName

    \section sec_RRD_Comm Общее описание RRDServer

    "RoundRobinDatabase" - реализация циклического хранилища.
    Процесс реализует циклическое хранение данных (от датчиков) и позволяет
    конфигурировать любое количество rrd-баз и входящих в них "источников".

    \section sec_RRD_Conf Настройка RRDServer

    Пример секции конфигурации:
\code
        <RRDServer1 name="RRDServer1">
            <rrd filename="rrdtest.rrd" filter_field="rrd" filter_value="1" step="5" ds_field="rrd1_ds" overwrite="0">
                <item rra="RRA:AVERAGE:0.5:1:4320"/>
                <item rra="RRA:MAX:0.5:1:4320"/>
            </rrd>
            <rrd filename="rrdtest2.rrd" filter_field="rrd" filter_value="2" step="10" ds_field="rrd2_ds" overwrite="0">
                <item rra="RRA:AVERAGE:0.5:1:4320"/>
                <item rra="RRA:MAX:0.5:1:4320"/>
            </rrd>
        </RRDServer1>
\endcode
    Где:
    - \b filename - имя создаваемого rrd-файла
    - \b filter_field - поле у датчика, определяющее, что его нужно сохранять в БД
    - \b filter_value - значение \b filter_field, определяющее, что датчик нужно сохранять в БД
    - \b ds_field - поле определяющее, параметр задающий формат хранения. Если \a ds_field не задано,
    то будет браться filter_field+filter_value+'_ds'.
    - \b step - период обновления данных (в секундах)
    - \b overwrite - [0,1]. Пересоздавать ли БД, если файл уже существует.

    При этом в секции <sensors> у датчиков прописываются параметры относящиеся к источнику:
\code
    <sensors>
        ...
        <item id="54" iotype="AI" name="AI54_S" textname="AI sensor 54" rrd="1" rrd1_ds="GAUGE:20:U:U"/>
        <item id="55" iotype="AI" name="AI55_S" textname="AI sensor 55" rrd="1" rrd1_ds="GAUGE:20:U:U"/>
        <item id="56" iotype="AI" name="AI56_S" textname="AI sensor 56" rrd="2" rrd2_ds="COUNTER:20:U:U"/>
		<item id="57" iotype="AI" name="AI57_S" rrd2_ds_dsname='A57MyRRDName' textname="AI sensor 57" rrd="2" rrd2_ds="DERIVE:20:U:U"/>
        ...
    </sensors>
\endcode

  \section sec_RRD_DSName Именование параметров
   По умолчанию в качестве имени параметра берётся поле \b 'ds_field'_dsname='', если это поле не указано, то берётся \b name датчика.
   \warning Имя не может превышать RRDServer::RRD_MAX_DSNAME_LEN.
*/
class RRDServer:
	public UObject_SK
{
	public:
		RRDServer( uniset::ObjectId objId, xmlNode* cnode, uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
				   const std::string& prefix = "rrd" );
		virtual ~RRDServer();

		/*! глобальная функция для инициализации объекта */
		static std::shared_ptr<RRDServer> init_rrdstorage( int argc, const char* const* argv,
				uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
				const std::string& prefix = "rrd" );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char* const* argv );

		inline std::shared_ptr<LogAgregator> getLogAggregator()
		{
			return loga;
		}
		inline std::shared_ptr<DebugStream> log()
		{
			return mylog;
		}

		const size_t RRD_MAX_DSNAME_LEN = 19; /*!< максимальная длинна имени в RRD */

	protected:
		RRDServer();

		virtual void askSensors( UniversalIO::UIOCommand cmd ) override;
		virtual void sensorInfo( const uniset::SensorMessage* sm ) override;
		virtual void timerInfo( const uniset::TimerMessage* tm ) override;
		virtual void sysCommand( const uniset::SystemMessage* sm ) override;

		void initRRD( xmlNode* cnode, int tmID );
		virtual void step() override;

		std::shared_ptr<SMInterface> shm;

		struct DSInfo
		{
			uniset::ObjectId sid;
			std::string dsname;
			long value;

			DSInfo( uniset::ObjectId id, const std::string& dsname, long defval ):
				sid(id), dsname(dsname), value(defval) {}
		};

		// Т.к. RRD требует чтобы данные записывались именно в том порядке в котором они были добавлены
		// при инициализации и при этом, нам нужен быстрый доступ в обработчике sensorInfo.
		// То создаём list<> для последовательного прохода по элементам в нужном порядке
		// и unordered_map<> для быстрого доступа к элементам в sensorInfo
		// При этом используем shared_ptr чтобы элементы указывали на один и тот же DSInfo
		typedef std::unordered_map<uniset::ObjectId, std::shared_ptr<DSInfo>> DSMap;
		typedef std::list<std::shared_ptr<DSInfo>> DSList;

		struct RRDInfo
		{
			std::string filename;
			long tid;
			long sec;
			DSMap dsmap;
			DSList dslist;

			RRDInfo( const std::string& fname, long tmID, long sec, const DSList& lst );
		};

		typedef std::list<RRDInfo> RRDList;

		RRDList rrdlist;

	private:

		std::string prefix;
};
// --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // _RRDServer_H_
// -----------------------------------------------------------------------------
