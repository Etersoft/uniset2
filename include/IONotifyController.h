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
// --------------------------------------------------------------------------
/*! \file
 * \brief Реализация IONotifyController_i
 * \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#ifndef IONotifyController_H_
#define IONotifyController_H_
//---------------------------------------------------------------------------
#include <memory>
#include <unordered_map>
#include <list>
#include <string>

#include "UniSetTypes.h"
#include "IOController_i.hh"
#include "IOController.h"

//---------------------------------------------------------------------------
class NCRestorer;
//---------------------------------------------------------------------------
/*!
    \page page_IONotifyController Хранение информации о состоянии с уведомлениями об изменении (IONotifyController)

    Класс IONotifyController расширяет набор задач класса IOController.
    Для ознакомления с базовыми функциями см. \ref page_IOController

    Задачи решаемые IONotifyController-ом (\b IONC):
    - \ref sec_NC_AskSensors
    - \ref sec_NC_Consumers
    - \ref sec_NC_Thresholds
    - \ref sec_NC_Depends

    \section sec_NC_AskSensors Механизм заказа датчиков
    Главной задачей класса IONotifyController является уведомление
объектов (заказчиков) об изменении состояния датчика (входа или выхода).

Механизм функционирует по следующей логике:
"заказчики" уведомляют \b IONC о том, об изменении какого именно датчика
они хотят получать уведомление.
После чего, если данный датчик меняет своё состояние, заказчику посылается
сообщение UniSetTypes::SensorMessage содержащее информацию о текущем(новом) состоянии датчика,
времени изменения и т.п. В случае необходимости можно отказатся от уведомления.
Для заказа датчиков предусмотрен ряд функций. На данный момент рекомендуется
пользоватся функцией IONotifyController::askSensor. Функции askState и askValue считаются устаревшими
и оставлены для совместимости со старыми интерфейсами.
... продолжение следует...

    \section sec_NC_Consumers  Заказчики
В качестве "заказчиков" могут выступать любые UniSet-объекты (UniSetObject),
обладающие "обратным адресом" (идентификатором), по которому присылается
уведомление об изменении состояния. Свой обратный адрес, объекты указывают
непосредственно при заказе (см. IONotifyController::askSensor).

Помимо "динамического" заказа во время работы процессов, существует возможность
задавать список заказчиков на этапе конфигурирования системы ("статический" способ).
Для этого в конфигурационном файле, в секции \b <sensors> у каждого датчика предусмотрена
специальная секция \b <consumers>.
\code
<sensors>
...
<item name="Sensors1" textname="sensor N1" iotype="AI" ...>
   <consumers>
       <consumer name="TestProc1" type="objects"/>
       <consumer name="TestProc2" type="managers" node="RemoteNode"/>
       ...
   </consumers>
</item>
...
</sensors>
\endcode
"Статический" способ заказа гарантирует, что при перезапуске
\b IONC список заказчиков будет восстановлен по конфигурационному файлу.

    \section sec_NC_Thresholds Пороговые датчики

    \section sec_NC_Depends Механизм зависимостей между датчиками
    Механизм зависимостей позволяет задать зависимость одного датчика, от другого.
Например пока "разрешающий" датчик не равен "1", у зависимого держится значение "0".
    Зависимость настраивается в конфигурационном файле, непосредственно у "зависимого датчика".
    <br>Доступные настройки:
  - \b depend            - задаёт имя датчика \b ОТ \b КОТОРОГО зависит данный
  - \b depend_value      - задаёт "разрешающее работу" значение (если поле не задано, считается depend_value="1").
  - \b depend_off_value  - задаёт значение которое будет выставлено для данного датчика, в случае его "блокировки". По умолчанию 0.

\code
    ...
    <sensors ..>
        ...
        <item id="11" iotype="AI" name="AI11_AS" textname="AI 11" depend="Input4_S" depend_off_value="-50"/>
        ...
    </sensors>
\endcode
    В данном примере можно увидеть, что датчик AI11_AS зависит от датчика Input4_S и пока Input4_S=0,
в AI11_AS будет записано значение -50. Как только Input4_S=1 в AI11_AS - появиться его истинное значение.

\note Следует иметь ввиду, что для \b ЗАВИСИМОГО датчика функция setValue(..) действует как обычно и
даже если он "заблокирован", значение в него можно сохранять. Оно "появиться" как только сниметься блокировка.
*/
//---------------------------------------------------------------------------
/*! \class IONotifyController
 * \todo Сделать логирование выходов

 \section AskSensors Заказ датчиков

    ....
    ConsumerMaxAttempts - максимальное число неудачных
попыток послать сообщение "заказчику". Настраивается в
конфигурационном файле. По умолчанию = 5.
*/
class IONotifyController:
	public IOController,
	public POA_IONotifyController_i
{
	public:

		IONotifyController(const std::string& name, const std::string& section, std::shared_ptr<NCRestorer> dumper = nullptr );
		IONotifyController(const UniSetTypes::ObjectId id, std::shared_ptr<NCRestorer> dumper = nullptr );

		virtual ~IONotifyController();

		virtual UniSetTypes::ObjectType getType() override
		{
			return UniSetTypes::ObjectType("IONotifyController");
		}
		virtual void askSensor(const UniSetTypes::ObjectId sid, const UniSetTypes::ConsumerInfo& ci, UniversalIO::UIOCommand cmd) override;

		virtual void askThreshold(const UniSetTypes::ObjectId sid, const UniSetTypes::ConsumerInfo& ci,
								  UniSetTypes::ThresholdId tid,
								  CORBA::Long lowLimit, CORBA::Long hiLimit, CORBA::Boolean invert,
								  UniversalIO::UIOCommand cmd ) override;

		virtual IONotifyController_i::ThresholdInfo getThresholdInfo( const UniSetTypes::ObjectId sid, UniSetTypes::ThresholdId tid ) override;
		virtual IONotifyController_i::ThresholdList* getThresholds(const UniSetTypes::ObjectId sid ) override;
		virtual IONotifyController_i::ThresholdsListSeq* getThresholdsList() override;

		virtual UniSetTypes::IDSeq* askSensorsSeq(const UniSetTypes::IDSeq& lst,
				const UniSetTypes::ConsumerInfo& ci, UniversalIO::UIOCommand cmd) override;

		// --------------------------------------------

		// функция для работы напрямую с указателем (оптимизация)
		virtual void localSetValue( std::shared_ptr<USensorInfo>& usi,
									UniSetTypes::ObjectId sid,
									CORBA::Long value, UniSetTypes::ObjectId sup_id ) override;
		// --------------------------------------------

		/*! Информация о заказчике */
		struct ConsumerInfoExt:
			public    UniSetTypes::ConsumerInfo
		{
			ConsumerInfoExt( const UniSetTypes::ConsumerInfo& ci,
							 UniSetObject_i_ptr ref = 0, int maxAttemtps = 10 ):
				UniSetTypes::ConsumerInfo(ci),
				ref(ref), attempt(maxAttemtps) {}

			UniSetObject_i_var ref;
			int attempt;

			ConsumerInfoExt( const ConsumerInfoExt& ) = default;
			ConsumerInfoExt& operator=( const ConsumerInfoExt& ) = default;
			ConsumerInfoExt( ConsumerInfoExt&& ) = default;
			ConsumerInfoExt& operator=(ConsumerInfoExt&& ) = default;
		};

		typedef std::list<ConsumerInfoExt> ConsumerList;

		struct ConsumerListInfo
		{
			ConsumerListInfo(): mut("ConsumerInfoMutex") {}
			ConsumerList clst;
			UniSetTypes::uniset_rwmutex mut;

			ConsumerListInfo( const ConsumerListInfo& ) = delete;
			ConsumerListInfo& operator=( const ConsumerListInfo& ) = delete;
			ConsumerListInfo( ConsumerListInfo&& ) = default;
			ConsumerListInfo& operator=(ConsumerListInfo&& ) = default;
		};

		/*! словарь: датчик -> список потребителей */
		typedef std::unordered_map<UniSetTypes::KeyType, ConsumerListInfo> AskMap;


		/*! Информация о пороговом значении */
		struct ThresholdInfoExt:
			public IONotifyController_i::ThresholdInfo
		{
			ThresholdInfoExt( UniSetTypes::ThresholdId tid, CORBA::Long low, CORBA::Long hi, bool inv,
							  UniSetTypes::ObjectId _sid = UniSetTypes::DefaultObjectId ):
				sid(_sid),
				invert(inv)
			{
				id       = tid;
				hilimit  = hi;
				lowlimit = low;
				state    = IONotifyController_i::NormalThreshold;
			}

			ConsumerListInfo clst; /*!< список заказчиков данного порога */

			/*! идентификатор дискретного датчика связанного с данным порогом */
			UniSetTypes::ObjectId sid;

			/*! итератор в списке датчиков (для оптимально-быстрого доступа) */
			IOController::IOStateList::iterator sit;

			/*! инверсная логика */
			bool invert;

			inline bool operator== ( const ThresholdInfo& r ) const
			{
				return ((id == r.id) &&
						(hilimit == r.hilimit) &&
						(lowlimit == r.lowlimit) &&
						(invert == r.invert) );
			}

			operator IONotifyController_i::ThresholdInfo()
			{
				IONotifyController_i::ThresholdInfo r;
				r.id = id;
				r.hilimit = hilimit;
				r.lowlimit = lowlimit;
				r.invert = invert;
				r.tv_sec = tv_sec;
				r.tv_usec = tv_usec;
				r.state = state;
				return r;
			}

			ThresholdInfoExt( const ThresholdInfoExt& ) = delete;
			ThresholdInfoExt& operator=( const ThresholdInfoExt& ) = delete;
			ThresholdInfoExt( ThresholdInfoExt&& ) = default;
			ThresholdInfoExt& operator=(ThresholdInfoExt&& ) = default;
		};

		/*! список порогов (информация по каждому порогу) */
		typedef std::list<ThresholdInfoExt> ThresholdExtList;

		struct ThresholdsListInfo
		{
			ThresholdsListInfo() {}
			ThresholdsListInfo( const IOController_i::SensorInfo& si, ThresholdExtList&& list,
								UniversalIO::IOType t = UniversalIO::AI ):
				si(si), type(t), list( std::move(list) ) {}

			UniSetTypes::uniset_rwmutex mut;

			IOController_i::SensorInfo si = { UniSetTypes::DefaultObjectId, UniSetTypes::DefaultObjectId };
			std::shared_ptr<USensorInfo> ait;
			UniversalIO::IOType type = { UniversalIO::AI };
			ThresholdExtList list;   /*!< список порогов по данному аналоговому датчику */
		};

		/*! словарь: аналоговый датчик --> список порогов по нему */
		typedef std::unordered_map<UniSetTypes::KeyType, ThresholdsListInfo> AskThresholdMap;

	protected:
		IONotifyController();
		virtual bool activateObject() override;
		virtual void initItem( IOStateList::iterator& it, IOController* ic );

		// ФИЛЬТРЫ
		bool myIOFilter(std::shared_ptr<USensorInfo>& ai, CORBA::Long newvalue, UniSetTypes::ObjectId sup_id);

		//! посылка информации об изменении состояния датчика
		virtual void send( ConsumerListInfo& lst, UniSetTypes::SensorMessage& sm );

		//! проверка срабатывания пороговых датчиков
		virtual void checkThreshold( std::shared_ptr<USensorInfo>& usi, const UniSetTypes::ObjectId sid, bool send = true );
		virtual void checkThreshold(IOController::IOStateList::iterator& li, const UniSetTypes::ObjectId sid, bool send_msg=true );

		//! поиск информации о пороговом датчике
		ThresholdExtList::iterator findThreshold( const UniSetTypes::ObjectId sid, const UniSetTypes::ThresholdId tid );

		//! сохранение информации об изменении состояния датчика в базу
		virtual void loggingInfo( UniSetTypes::SensorMessage& sm );

		/*! сохранение списка заказчиков
		    По умолчанию делает dump, если объявлен dumper.
		*/
		virtual void dumpOrdersList( const UniSetTypes::ObjectId sid, const IONotifyController::ConsumerListInfo& lst );

		/*! сохранение списка заказчиков пороговых датчиков
		    По умолчанию делает dump, если объявлен dumper.
		*/
		virtual void dumpThresholdList( const UniSetTypes::ObjectId sid, const IONotifyController::ThresholdExtList& lst );

		/*! чтение dump-файла */
		virtual void readDump();

		std::shared_ptr<NCRestorer> restorer;

		void onChangeUndefinedState( std::shared_ptr<USensorInfo>& it, IOController* ic );

	private:
		friend class NCRestorer;

		//----------------------
		bool addConsumer(ConsumerListInfo& lst, const UniSetTypes::ConsumerInfo& cons );     //!< добавить потребителя сообщения
		bool removeConsumer(ConsumerListInfo& lst, const UniSetTypes::ConsumerInfo& cons );  //!< удалить потребителя сообщения

		//! обработка заказа
		void ask(AskMap& askLst, const UniSetTypes::ObjectId sid,
				 const UniSetTypes::ConsumerInfo& ci, UniversalIO::UIOCommand cmd);

		/*! добавить новый порог для датчика */
		bool addThreshold(ThresholdExtList& lst, ThresholdInfoExt&& ti, const UniSetTypes::ConsumerInfo& ci);
		/*! удалить порог для датчика */
		bool removeThreshold(ThresholdExtList& lst, ThresholdInfoExt& ti, const UniSetTypes::ConsumerInfo& ci);

		AskMap askIOList; /*!< список потребителей по аналоговым датчикам */
		AskThresholdMap askTMap; /*!< список порогов по аналоговым датчикам */

		/*! замок для блокирования совместного доступа к cписку потребителей датчиков */
		UniSetTypes::uniset_rwmutex askIOMutex;
		/*! замок для блокирования совместного доступа к cписку потребителей пороговых датчиков */
		UniSetTypes::uniset_rwmutex trshMutex;

		int maxAttemtps; /*! timeout for consumer */

		sigc::connection conInit;
		sigc::connection conUndef;
};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
