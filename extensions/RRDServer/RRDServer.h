#ifndef _RRDServer_H_
#define _RRDServer_H_
// -----------------------------------------------------------------------------
#include <unordered_map>
#include <memory>
#include "UObject_SK.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "extensions/Extensions.h"
// -----------------------------------------------------------------------------
/*!
    \page page_RRDServer Реализация RRD хранилища

      - \ref sec_RRD_Comm
      - \ref sec_RRD_Conf

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
        <item id="57" iotype="AI" name="AI57_S" textname="AI sensor 57" rrd="2" rrd2_ds="DERIVE:20:U:U"/>
        ...
    </sensors>
\endcode
*/
class RRDServer:
    public UObject_SK
{
    public:
        RRDServer( UniSetTypes::ObjectId objId, xmlNode* cnode, UniSetTypes::ObjectId shmID, const std::shared_ptr<SharedMemory> ic=nullptr,
                    const std::string& prefix="rrd", std::shared_ptr<DebugStream> log=UniSetExtensions::dlog() );
        virtual ~RRDServer();

        /*! глобальная функция для инициализации объекта */
        static std::shared_ptr<RRDServer> init_rrdstorage( int argc, const char* const* argv,
                            UniSetTypes::ObjectId shmID, const std::shared_ptr<SharedMemory> ic=nullptr,
                            const std::string& prefix="rrd" );

        /*! глобальная функция для вывода help-а */
        static void help_print( int argc, const char* const* argv );

    protected:
        RRDServer();

        virtual void askSensors( UniversalIO::UIOCommand cmd ) override;
        virtual void sensorInfo( const UniSetTypes::SensorMessage* sm ) override;
        virtual void timerInfo( const UniSetTypes::TimerMessage* tm ) override;
        virtual void sysCommand( const UniSetTypes::SystemMessage* sm ) override;

        void initRRD( xmlNode* cnode, int tmID );
        virtual void step() override;

        std::shared_ptr<SMInterface> shm;

        struct DSInfo
        {
            std::string dsname;
            long value;

            DSInfo( const std::string& dsname, long defval ):
                dsname(dsname),value(defval){}
        };

        typedef std::unordered_map<UniSetTypes::ObjectId,DSInfo> DSMap;

        struct RRDInfo
        {
            std::string filename;
            long tid;
            long sec;
            DSMap dsmap;

            RRDInfo( const std::string& fname, long tmID, long sec, const DSMap& ds ):
                filename(fname),tid(tmID),sec(sec),dsmap(ds){}
        };

        typedef std::list<RRDInfo> RRDList;

        RRDList rrdlist;

    private:

        std::string prefix;
};
// -----------------------------------------------------------------------------
#endif // _RRDServer_H_
// -----------------------------------------------------------------------------
