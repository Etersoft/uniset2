// -------------------------------------------------------------------------- 
#ifndef ObjectIndex_idXML_H_
#define ObjectIndex_idXML_H_
// --------------------------------------------------------------------------
#include <map>
#include <string>
#include "ObjectIndex.h"
#include "UniXML.h"
// --------------------------------------------------------------------------
class ObjectIndex_idXML:
	public UniSetTypes::ObjectIndex
{
	public:
		ObjectIndex_idXML( const std::string xmlfile );
		ObjectIndex_idXML(UniXML& xml);
		virtual ~ObjectIndex_idXML();

		virtual const UniSetTypes::ObjectInfo* getObjectInfo( const UniSetTypes::ObjectId );
		virtual const UniSetTypes::ObjectInfo* getObjectInfo( const std::string name );
		virtual UniSetTypes::ObjectId getIdByName( const std::string& name );
		virtual std::string getMapName( const UniSetTypes::ObjectId id );
		virtual std::string getTextName( const UniSetTypes::ObjectId id );

		virtual std::ostream& printMap( std::ostream& os );
		friend std::ostream& operator<<(std::ostream& os, ObjectIndex_idXML& oi );
	
	protected:
		virtual void build( UniXML& xml );
		void read_section( UniXML& xml, const std::string sec );
		void read_nodes( UniXML& xml, const std::string sec );
	
	private:
		typedef std::map<UniSetTypes::ObjectId, UniSetTypes::ObjectInfo> MapObjects;
		MapObjects omap;

		typedef std::map<std::string, UniSetTypes::ObjectId> MapObjectKey;
		MapObjectKey mok; // для обратного писка
};
// -----------------------------------------------------------------------------------------
#endif
