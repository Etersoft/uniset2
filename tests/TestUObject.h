#ifndef TestUObject_H_
#define TestUObject_H_
// -------------------------------------------------------------------------
#include "UniSetObject.h"
// -------------------------------------------------------------------------
/*! Специальный тестовый объект для тестирования класса UniSetObject
 * Для наглядности и простоты все функции объявлены здесь же в h-файле
*/
class TestUObject:
	public UniSetObject
{
	public:

		TestUObject( UniSetTypes::ObjectId id, xmlNode* cnode ):
			UniSetObject(id){}

		virtual ~TestUObject(){};

		// специальные функции для проведения тестирования
		inline VoidMessagePtr getOneMessage()
		{
			return receiveMessage();
		}

		inline bool mqEmpty()
		{
			return (countMessages() == 0);
		}

	protected:
		TestUObject(){};
};
// -------------------------------------------------------------------------
#endif // TestUObject_H_
// -------------------------------------------------------------------------
