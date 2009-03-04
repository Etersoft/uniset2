// $Id: ComediInterface.h,v 1.1 2008/12/14 21:57:50 vpashka Exp $
// -----------------------------------------------------------------------------
#ifndef ComediInterface_H_
#define ComediInterface_H_
// -----------------------------------------------------------------------------
#include <string>
#include <vector>
#include <comedilib.h>
#include "Exceptions.h"
// -----------------------------------------------------------------------------
/*! Интерфейс для работы с в/в */
class ComediInterface
{
	public:
		ComediInterface( const std::string dev );
		~ComediInterface();

		int getAnalogChannel( int subdev, int channel, int range=0, int aref=AREF_GROUND ) 
				throw(UniSetTypes::Exception);

		std::vector<lsampl_t> getAnalogPacket( int subdev, int channel, int range=0, int aref=AREF_GROUND ) 
				throw(UniSetTypes::Exception);

		void setAnalogChannel( int subdev, int channel, int data, int range=0, int aref=AREF_GROUND ) 
				throw(UniSetTypes::Exception);

		bool getDigitalChannel( int subdev, int channel )
				throw(UniSetTypes::Exception);

		void setDigitalChannel( int subdev, int channel, bool bit )
				throw(UniSetTypes::Exception);


		// Конфигурирование входов / выходов
		enum ChannelType
		{
			DI 	= INSN_CONFIG_DIO_INPUT,
			DO 	= INSN_CONFIG_DIO_OUTPUT,
			AI	= INSN_CONFIG_AIO_INPUT,
			AO	= INSN_CONFIG_AIO_OUTPUT
		};

		enum SubdevType
		{
			Unknown = 0,
			TBI24_0 = 1,
			TBI0_24 = 2,
			TBI16_8 = 3
		};

		enum EventType
		{
			No = 0,
			Front = 1,
			Rear = 2,
			FrontThenRear = 3
		};

		static std::string type2str( SubdevType t );
		static SubdevType str2type( const std::string s );
		static EventType event2type( const std::string s );
		static lsampl_t instr2type( const std::string s );

		void configureSubdev( int subdev, SubdevType type )	throw(UniSetTypes::Exception);

		void configureChannel( int subdev, int channel, ChannelType type, int range=0, int aref=0 )
				throw(UniSetTypes::Exception);

		/* Perform extended instruction at the channel. Arguments are being taken as C++ vector. */
		void instrChannel( int subdev, int channel, const std::string instr, std::vector<lsampl_t> args, int range=0, int aref=0 )
				throw(UniSetTypes::Exception);

		inline const std::string devname(){ return dname; }

	protected:

		comedi_t* card;	/*!< интерфейс для работы с картами в/в */
		std::string dname;

	private:
};
// -----------------------------------------------------------------------------
#endif // ComediInterface_H_
// -----------------------------------------------------------------------------

