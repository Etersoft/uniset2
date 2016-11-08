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
#ifndef UniExchange_H_
#define UniExchange_H_
// -----------------------------------------------------------------------------
#include <list>
#include <memory>
#include "UniXML.h"
#include "IOController.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "PassiveTimer.h"
// --------------------------------------------------------------------------
namespace uniset
{
// -----------------------------------------------------------------------------
/*!
        \page pageUniExchange Обмен между узлами на основе TCP.

    \par Обмен построен на основе функций IOControl-ера получения списка дискретных
    и аналоговых датчиков. Работает через удалённые CORBA-вызовы.

    \par Процесс считывает из конфигурационного файла список узлов которые необходимо
    опрашивать (точнее список IOControl-еров), запускается поток обмена, в котором
    эти узлы ПОСЛЕДОВАТЕЛЬНО опрашиваются..

    \par Пример записи в конфигурационном файле для опроса пяти узлов...
    \code
        <UniExchange name="UniExchange">
            <item name="UniExchange2" node="Node2"/>
            <item id="3001" node_id="Node2"/>
            <item name="UniExchange3" node="Node3"/>
            <item name="UniExchange4" node="Node4"/>
            <item name="UniExchange5" node="Node5"/>
        </UniExchange>
    \endcode
    Запись можно делать по "id" или по "name"
*/
// -----------------------------------------------------------------------------
class UniExchange:
	public IOController
{
	public:
		UniExchange( uniset::ObjectId id, uniset::ObjectId shmID,
					 const std::shared_ptr<SharedMemory>& ic = nullptr, const std::string& prefix = "unet" );
		virtual ~UniExchange();

		void execute();

		static std::shared_ptr<UniExchange> init_exchange( int argc, const char* const* argv,
				uniset::ObjectId shmID, const std::shared_ptr<SharedMemory>& ic = nullptr,
				const std::string& prefix = "unet" );

		/*! глобальная функция для вывода help-а */
		static void help_print( int argc, const char** argv );

		virtual IOController_i::ShortMapSeq* getSensors() override;

	protected:

		virtual void sysCommand( const uniset::SystemMessage* sm ) override;
		virtual void askSensors( UniversalIO::UIOCommand cmd );
		virtual void sigterm( int signo ) override;

		xmlNode* cnode = { 0 };
		std::string s_field = { "" };
		std::string s_fvalue = { "" };
		std::shared_ptr<SMInterface> shm;

		struct SInfo
		{
			// т.к. содержится rwmutex с запрещённым конструктором копирования
			// приходится здесь тоже объявлять разрешенными только операции "перемещения"
			SInfo( const SInfo& r ) = delete;
			SInfo& operator=(const SInfo& r) = delete;
			SInfo( SInfo&& r ) = default;
			SInfo& operator=(SInfo&& r) = default;

			SInfo():
				val(0),
				id(uniset::DefaultObjectId),
				type(UniversalIO::UnknownIOType)
			{}

			IOController::IOStateList::iterator ioit;
			long val;
			long id;
			UniversalIO::IOType type;
			uniset::uniset_rwmutex val_lock;
		};

		typedef std::vector<SInfo> SList;

		struct NetNodeInfo
		{
			// т.к. содержится SList в котором rwmutex с запрещённым конструктором копирования
			// приходится здесь тоже объявлять разрешенными только операции "перемещения"
			NetNodeInfo( const NetNodeInfo& r ) = delete;
			NetNodeInfo& operator=(const NetNodeInfo& r) = delete;
			NetNodeInfo( NetNodeInfo&& r ) = default;
			NetNodeInfo& operator=(NetNodeInfo&& r) = default;

			NetNodeInfo();

			CORBA::Object_var oref;
			IOController_i_var shm;
			uniset::ObjectId id;
			uniset::ObjectId node;
			uniset::ObjectId sidConnection; /*!< датчик связи */
			IOController::IOStateList::iterator conn_it;
			SList smap;

			void update(IOController_i::ShortMapSeq_var& map, const std::shared_ptr<SMInterface>& shm );
		};

		typedef std::list<NetNodeInfo> NetNodeList;
		NetNodeList nlst;

		void readConfiguration();
		bool readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec );
		bool initItem( UniXML::iterator& it );
		void updateLocalData();
		void initIterators();

		timeout_t polltime = { 200 };
		PassiveTimer ptUpdate;
		bool init_ok = { false };

		SList mymap;
		size_t maxIndex = { 0 };
		timeout_t smReadyTimeout = { 15000 }; // msec

	private:
};
// --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // UniExchange_H_
