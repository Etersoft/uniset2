#ifndef UDPNReceiver_H_
#define UDPNReceiver_H_
// -----------------------------------------------------------------------------
#include <ostream>
#include <string>
#include <vector>
#include <cc++/socket.h>
#include "Mutex.h"
#include "Trigger.h"
#include "SMInterface.h"
#include "SharedMemory.h"
#include "ThreadCreator.h"
#include "UDPPacket.h"
// -----------------------------------------------------------------------------
class UDPNReceiver
{
	public:
		UDPNReceiver( ost::tpport_t port, ost::IPV4Host host, UniSetTypes::ObjectId shmID, IONotifyController* ic=0 );
		virtual ~UDPNReceiver();

		inline int getPort(){ return port; }
		inline bool isConnetcion(){ return conn; }

		inline void start(){ activate = true; }
		inline void stop(){ activate = false; }
		inline void setReceiveTimeout( int t ){ recvTimeout = t; }
		inline std::string getName(){ return myname; }

		/*! глобальная функция для инициализации объекта */
		static UDPNReceiver* init_udpreceiver( int argc, char* argv[],
											UniSetTypes::ObjectId shmID, SharedMemory* ic=0 );

	protected:
		SMInterface* shm;
		UniversalInterface ui;

		void poll();
		void recv();

		std::string myname;

	private:
		UDPNReceiver();

		bool activate;
		ost::UDPDuplex* udp;
		ost::IPV4Host host;
		ost::tpport_t port;

		int recvTimeout;
		bool conn;

		ThreadCreator<UDPNReceiver>* thr;
};
// -----------------------------------------------------------------------------
#endif // UDPNReceiver_H_
// -----------------------------------------------------------------------------
