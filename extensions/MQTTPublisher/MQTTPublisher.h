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
// -------------------------------------------------------------------------
namespace uniset
{
	// -----------------------------------------------------------------------------
	/*!
		\page page_MQTTPublisher Реализация MQTT издателя

		  - \ref sec_MQTT_Comm
		  - \ref sec_MQTT_Conf
		  - \ref sec_MQTT_Text

		\section sec_MQTT_Comm Общее описание MQTTPublisher

		MQTT - это..

		Данная реализация построена на использованиие библиотеки mosquitto.
		Издатель публикует события по каждому изменению датчика в указанном топике.

		\section sec_MQTT_Conf Настройка MQTTPublisher

		Топик для публикации событий имеет вид: TOPIC
		- \b TOPIC - заданный topic.

		Топик можно задать при помощи аргумента комадной строки --prefix-mqtt-topic или в настроечной секции topic="..".
		По умолчанию берётся ROOTPROJECT/sensors/sensorname, где
		ROOTPROJECT - это название корневой uniset-секции заданное в configure.xml (RootSection="..")

		События по каким датчикам "публиковать" можно задавать при помощи filter-field и filter-value параметров.
		--prefix-filter-field - задаёт фильтрующее поле для датчиков
		--prefix-filter-value - задаётзначение фильтрующего поля для датчиков. Необязательнй параметр.

		Либо можно указать в настроечной секции: filterField=".." filterValue=".."

		По умолчанию загружаются и публикуются ВСЕ датчики из секции <sensors> конфигурационного файла.

		Сервер для публикации указывается параметрами:
		--prefix-mqtt-host ip|hostname - По умолчаню "localhost"
		--prefix-mqtt-port num  - По умолчанию: 1883 (mosquitto)

		Но можно задать и в настроечной секции: mqttHost=".." и mqttPort=".."

		Помимо этого можно задать время проверки соединения, параметром
		--prefix-mqtt-keepalive sec - По умолчанию: 60 или и в настроечной секции: mqttKeepAlive=".."

		Для запуска издателя, неоходимо наличие в configure.xml секции: <ObjectName name="ObjectName" ...параметры">.

		\todo Доделать контрольный таймер (контроль наличия соединения с сервером)

		\section sec_MQTT_Text Генерирование текстовых сообщений
		Имеется возможность сопоставлять значения датчиков текстовым сообщениям, посылаемым на сервер.
		Для этого необходимо в настроечной секции для датчика создать подсекцию <mqtt>. Пример:
		\code
		<item id="10" name="MySensor1" filter_field="filter_value"...>
			<mqtt subtopic="myevent">
				<msg value="12" text="My text for value %v"/>
				<msg value="13" text="My text for value %v"/>
				<msg value="14" text="My text for value %v"/>
				<range min="10" max="20" text="My text for range %r. %n = %v"/>
			<mqtt>
		</item>
		\endcode
		 - \b range - задаёт диапазон включающий [min,max]
		 - \b subtopic - задаёт подраздел в корневом топике (см. topicsensors). Т.е. полный топик для публикации текстовых сообщений
		будет иметь вид ROOTPROJECT/topicsensors/sensorname/textevent или если задано поле \subtopic то
		события будут опубликованы в ROOTPROJECT/topicsensors/subtopic

		При этом в тексте можно применять следующие "подстановки":
		- \b %v - текущее значение (value)
		- \b %n - name
		- \b %t - textname
		- \b %i - ID
		- \b %r - заданный диапазон (range). Заменяется на "[min:max]"
		- \b %rmin - минимальное значение диапазона (range min)
		- \b %rmax - максимальное значение диапазона (range max)

		\note Если заданные "одиночные" значения совпадают с диапазоном, то будет сгенерировано несколько сообщений. Т.е. диапазоны могут пересекатся.

	*/
	class MQTTPublisher:
		protected mosqpp::mosquittopp,
		public UObject_SK
	{
		public:
			MQTTPublisher( uniset::ObjectId objId, xmlNode* cnode, uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
						   const std::string& prefix = "mqtt" );
			virtual ~MQTTPublisher();

			/*! глобальная функция для инициализации объекта */
			static std::shared_ptr<MQTTPublisher> init_mqttpublisher( int argc, const char* const* argv,
					uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
					const std::string& prefix = "mqtt" );

			/*! глобальная функция для вывода help-а */
			static void help_print( int argc, const char* const* argv );

			virtual void on_connect(int rc) override;
			virtual void on_message(const struct mosquitto_message* message) override;
			virtual void on_subscribe(int mid, int qos_count, const int* granted_qos) override;

		protected:
			MQTTPublisher();

			virtual void askSensors( UniversalIO::UIOCommand cmd ) override;
			virtual void sensorInfo( const uniset::SensorMessage* sm ) override;
			virtual bool deactivateObject() override;
			virtual void sysCommand( const uniset::SystemMessage* sm ) override;

			std::shared_ptr<SMInterface> shm;

			struct MQTTInfo
			{
				uniset::ObjectId sid;
				std::string pubname;

				MQTTInfo( uniset::ObjectId id, const std::string& name ):
					sid(id), pubname(name) {}
			};

			typedef std::unordered_map<uniset::ObjectId, MQTTInfo> MQTTMap;

			struct RangeInfo
			{
				RangeInfo( long min, long max, const std::string& t ): rmin(min), rmax(max), text(t) {}

				long rmin;
				long rmax;
				std::string text;
				bool check( long val ) const;
			};

			struct MQTTTextInfo
			{
				uniset::ObjectId sid;
				std::string pubname;
				UniXML::iterator xmlnode;

				MQTTTextInfo( const std::string& rootsec, UniXML::iterator s, UniXML::iterator i );

				// одиночные сообщения просто имитируются min=max=val
				std::list<RangeInfo> rlist; // список сообщений..

				void check( mosqpp::mosquittopp* serv, long value, std::shared_ptr<DebugStream>& log, const std::string& myname );

				std::string replace( RangeInfo* ri, long value );
			};

			typedef std::unordered_map<uniset::ObjectId, MQTTTextInfo> MQTTTextMap;

			MQTTMap publist;
			MQTTTextMap textpublist;

		private:

			std::string prefix;
			std::string topic; // "топик" для публикации датчиков
			bool connectOK = { false };
			std::string host = { "localhost" };
			int port = { 1883 };
			int keepalive = { 60 };
	};
	// ----------------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // _MQTTPublisher_H_
// -----------------------------------------------------------------------------
