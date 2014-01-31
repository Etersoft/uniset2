#ifndef LProcessor_H_
#define LProcessor_H_
// --------------------------------------------------------------------------
#include <map>
#include "UniSetTypes.h"
#include "UniversalInterface.h"
#include "Element.h"
#include "Schema.h"
// --------------------------------------------------------------------------
class LProcessor
{
	public:
		LProcessor();
	    virtual ~LProcessor();

		virtual void execute( const string lfile );

	protected:
		
		virtual void build( const string& lfile );
		
		virtual void step();
		
		virtual void getInputs();
		virtual void processing();
		virtual void setOuts();

		struct EXTInfo
		{
			UniSetTypes::ObjectId sid;
			UniversalIO::IOTypes iotype;
			bool state;
			const Schema::EXTLink* lnk;
		};

		struct EXTOutInfo
		{
			UniSetTypes::ObjectId sid;
			UniversalIO::IOTypes iotype;
			const Schema::EXTOut* lnk;
		};

		typedef std::list<EXTInfo> EXTList;
		typedef std::list<EXTOutInfo> OUTList;

		EXTList extInputs;
		OUTList extOuts;
		SchemaXML sch;

		UniversalInterface ui;
		int sleepTime;
		
	private:
	
	
};
// ---------------------------------------------------------------------------
#endif
