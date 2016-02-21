#ifndef _MQTTPublisher_H_
#define _MQTTPublisher_H_
// -----------------------------------------------------------------------------
#include <unordered_map>
#include <list>
#include <memory>
#include <mosquittopp.h>
#include "UObject_SK.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "extensions/Extensions.h"
// -----------------------------------------------------------------------------
/*!
	\page page_MQTTPublisher Реализация MQTT издателя

	  - \ref sec_MQTT_Comm
	  - \ref sec_MQTT_Conf

	\section sec_MQTT_Comm Общее описание MQTTPublisher

	MQTT - это..

	Данная реализация построена на использованиие билиотеки mosquitto.
	Издатель публикует события по каждому изменению датчика в указанном топике.

	\section sec_MQTT_Conf Настройка MQTTPublisher

	Топик для публикации событий имеет вид: ROOTPROJECT/topicsensors/sensorname, где
	- \b ROOTPROJECT - это название корневой uniset-секции заданное в configure.xml (RootSection="..")
	- \b topicsensors - это название секции для публикации в MQTT-сервере (брокере).
	Название можно задать при помощи аргумента конмадной строки --prefix-mqtt-topicsensors
	или в настроечной секции topicsensors="..". По умолчанию topicsensors='sensors'.

	Какие датчики "публиковать" можно задавать при помощи filter-field и filter-value параметров.
	--prefix-filter-field - задаёт фильтрующее поле для датчиков
	--prefix-filter-value - задаётзначение фильтрующего поля для датчиков. Необязательнй параметр.

	Либо можно указать в настроечной секции: filterField=".." filterValue=".."

	По умолчанию загружаются и публикуются ВСЕ датчики из секции <sensors> конфигурационного файла.

	Сервер для публикации указывается параметрами:
	--prefix-mqtt-host ip|hostname - По умолчаню "localhost"
	--prefix-mqtt-port num  - По умолчанию: 1883 (mosquitto)

	Но можно задать и в настроечной секции: mqttHost=".." и mqttPort=".."

	Помимо этого можно задать время проверки соединения, параметром
	--prefix-mqtt-keepalive sec - По умолчанию: 60
	или и в настроечной секции: mqttKeepAlive=".."

	Для запуска издателя, неоходимо наличие в configure.xml секции: <ObjectName name="ObjectName" ...параметры">.

	\todo Доделать контрольный таймер (контроль наличия соединения с сервером)
*/
class MQTTPublisher:
	protected mosqpp::mosquittopp,
	public UObject_SK
{
	public:
		MQTTPublisher( UniSetTypes::ObjectId objId, xmlNode* cnode, UniSetTypes::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
					   const std::string& prefix = "mqtt" );
		virtual ~MQTTPublisher();

		/*! глобальная функция для инициализации объекта */
		static std::shared_ptr<MQTTPublisher> init_mqttpublisher( int argc, const char* const* argv,
				UniSetTypes::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
				const std::string& prefix = "mqtt" );

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


		virtual void on_connect(int rc) override;
		virtual void on_message(const struct mosquitto_message* message) override;
		virtual void on_subscribe(int mid, int qos_count, const int* granted_qos) override;

	protected:
		MQTTPublisher();

		virtual void askSensors( UniversalIO::UIOCommand cmd ) override;
		virtual void sensorInfo( const UniSetTypes::SensorMessage* sm ) override;
		virtual void sigterm( int signo ) override;
		virtual bool deactivateObject() override;
		virtual void sysCommand( const UniSetTypes::SystemMessage* sm ) override;

		std::shared_ptr<SMInterface> shm;

		struct MQTTInfo
		{
			UniSetTypes::ObjectId sid;
			std::string pubname;

			MQTTInfo( UniSetTypes::ObjectId id, const std::string& name ):
				sid(id), pubname(name) {}
		};

		typedef std::unordered_map<UniSetTypes::ObjectId, MQTTInfo> MQTTMap;

		MQTTMap publist;

	private:

		std::string prefix;
		std::string topicsensors; // "топик" для публикации датчиков
		bool connectOK = { false };
		std::string host = { "localhost" };
		int port = { 1883 };
		int keepalive = { 60 };
};
// -----------------------------------------------------------------------------
#endif // _MQTTPublisher_H_
// -----------------------------------------------------------------------------
