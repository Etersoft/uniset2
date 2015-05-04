#ifndef Schema_H_
#define Schema_H_
// --------------------------------------------------------------------------
#include <memory>
#include <unordered_map>
#include "Element.h"
#include "Schema.h"
// --------------------------------------------------------------------------
class Schema
{
	public:
		Schema();
		virtual ~Schema();

		std::shared_ptr<Element> manage( std::shared_ptr<Element> el );

		void remove( std::shared_ptr<Element> el );

		void link( Element::ElementID rootID, Element::ElementID childID, int numIn );
		void unlink( Element::ElementID rootID, Element::ElementID childID );
		void extlink( const std::string& name, Element::ElementID childID, int numIn );

		void setIn( Element::ElementID ID, int inNum, bool state );
		bool getOut( Element::ElementID ID );

		struct INLink;
		struct EXTLink;
		struct EXTOut;

		typedef std::unordered_map<Element::ElementID, std::shared_ptr<Element>> ElementMap;
		typedef std::list<INLink> InternalList;
		typedef std::list<EXTLink> ExternalList;
		typedef std::list<EXTOut> OutputsList;

		// map iterator
		typedef ElementMap::const_iterator iterator;
		inline Schema::iterator begin()
		{
			return emap.begin();
		}
		inline Schema::iterator end()
		{
			return emap.end();
		}
		inline int size()
		{
			return emap.size();
		}
		inline bool empty()
		{
			return emap.empty();
		}

		// int. list iterator
		typedef InternalList::const_iterator INTiterator;
		inline Schema::INTiterator intBegin()
		{
			return inLinks.begin();
		}
		inline Schema::INTiterator intEnd()
		{
			return inLinks.end();
		}
		inline int intSize()
		{
			return inLinks.size();
		}
		inline bool intEmpty()
		{
			return inLinks.empty();
		}

		// ext. list iterator
		typedef ExternalList::const_iterator EXTiterator;
		inline Schema::EXTiterator extBegin()
		{
			return extLinks.begin();
		}
		inline Schema::EXTiterator extEnd()
		{
			return extLinks.end();
		}
		inline int extSize()
		{
			return extLinks.size();
		}
		inline bool extEmpty()
		{
			return extLinks.empty();
		}

		// ext. out iterator
		typedef OutputsList::const_iterator OUTiterator;
		inline Schema::OUTiterator outBegin()
		{
			return outList.begin();
		}
		inline Schema::OUTiterator outEnd()
		{
			return outList.end();
		}
		inline int outSize()
		{
			return outList.size();
		}
		inline bool outEmpty()
		{
			return outList.empty();
		}

		// find
		std::shared_ptr<Element> find(Element::ElementID id);
		std::shared_ptr<Element> findExtLink(const std::string& name);
		std::shared_ptr<Element> findOut(const std::string& name);

		// -----------------------------------------------
		// внутренее соединения
		// между элементами
		struct INLink
		{
			INLink(std::shared_ptr<Element> f, std::shared_ptr<Element> t, int ni):
				numInput(ni) {}
			INLink(): numInput(0) {}

			std::shared_ptr<Element> from;
			std::shared_ptr<Element> to;
			int numInput;
		};

		// внешнее соединение
		// что-то на вход элемента
		struct EXTLink
		{
			EXTLink( const std::string& n, std::shared_ptr<Element> t, int ni):
				name(n), to(t), numInput(ni) {}
			EXTLink(): name(""), numInput(0) {}

			std::string name;
			std::shared_ptr<Element> to;
			int numInput;
		};

		// наружный выход
		struct EXTOut
		{
			EXTOut( const std::string n, std::shared_ptr<Element> f):
				name(n), from(f) {}
			EXTOut(): name("") {}

			std::string name;
			std::shared_ptr<Element> from;
		};

	protected:
		ElementMap emap; // список элеметов
		InternalList inLinks;
		ExternalList extLinks;
		OutputsList outList;

	private:
};
// ---------------------------------------------------------------------------
class SchemaXML:
	public Schema
{
	public:
		SchemaXML();
		virtual ~SchemaXML();

		void read( const std::string& xmlfile );

	protected:
};
// ---------------------------------------------------------------------------
#endif
