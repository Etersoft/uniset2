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
 * \brief Активатор объектов
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef UniSetActivator_H_
#define UniSetActivator_H_
// --------------------------------------------------------------------------
#include <deque>
#include <memory>
#include <omniORB4/CORBA.h>
#include <cc++/socket.h>
#include "UniSetTypes.h"
#include "UniSetObject.h"
#include "UniSetManager.h"
#include "OmniThreadCreator.h"
//----------------------------------------------------------------------------------------
class UniSetActivator;
typedef std::shared_ptr<UniSetActivator> UniSetActivatorPtr;
//----------------------------------------------------------------------------------------
/*! \class UniSetActivator
 *    Создает POA менеджер и регистрирует в нем объекты.
 *    Для обработки CORBA-запросов создается поток или передаются ресурсы
 *        главного потока см. void activate(bool thread)
 *    \warning Авктиватор может быть создан только один. Для его создания используйте код:
  \code
     ...
     auto act = UniSetActivator::Instance()
     ...
\endcode
 * Активатор в свою очередь сам является менеджером(и объектом) и обладает всеми его свойствами
 *
 * --uniset-no-use-gdb-for-stacktrace - НЕ ИСПОЛЬЗОВАТЬ gdb для stacktrace
*/
class UniSetActivator:
	public UniSetManager
{
	public:

		static UniSetActivatorPtr Instance( const UniSetTypes::ObjectId id = UniSetTypes::DefaultObjectId );
		void Destroy();

		std::shared_ptr<UniSetActivator> get_aptr()
		{
			return std::dynamic_pointer_cast<UniSetActivator>(get_ptr());
		}
		// ------------------------------------
		virtual ~UniSetActivator();

		virtual void run(bool thread);
		virtual void stop();
		virtual void uaDestroy(int signo = 0);

		virtual UniSetTypes::ObjectType getType() override
		{
			return UniSetTypes::ObjectType("UniSetActivator");
		}

		typedef sigc::signal<void, int> TerminateEvent_Signal;
		TerminateEvent_Signal signal_terminate_event();

		inline bool noUseGdbForStackTrace()
		{
			return _noUseGdbForStackTrace;
		}

	protected:

		virtual void work();

		inline CORBA::ORB_ptr getORB()
		{
			return orb;
		}

		virtual void sysCommand( const UniSetTypes::SystemMessage* sm ) override;

		// уносим в protected, т.к. Activator должен быть только один..
		UniSetActivator();
		UniSetActivator( const UniSetTypes::ObjectId id );
		static std::shared_ptr<UniSetActivator> inst;

	private:
		friend void terminate_thread();
		friend void finished_thread();
		friend std::shared_ptr<UniSetTypes::Configuration> UniSetTypes::uniset_init( int argc, const char* const* argv, const std::string& xmlfile );

		static void terminated(int signo);
		static void normalexit();
		static void normalterminate();
		static void set_signals(bool ask);
		void term( int signo );
		void init();

		std::shared_ptr< OmniThreadCreator<UniSetActivator> > orbthr;

		CORBA::ORB_var orb;
		TerminateEvent_Signal s_term;

		std::atomic_bool omDestroy;
		pid_t thpid; // pid orb потока

		bool _noUseGdbForStackTrace = { false };
};
//----------------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------------
