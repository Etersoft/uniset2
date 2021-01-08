#ifndef TestUObject_H_
#define TestUObject_H_
// -------------------------------------------------------------------------
#include "UniSetObject.h"
// -------------------------------------------------------------------------
/*! Специальный тестовый объект для тестирования класса UniSetObject
 * Для наглядности и простоты все функции объявлены здесь же в h-файле
*/
class TestUObject:
    public uniset::UniSetObject
{
    public:

        TestUObject( uniset::ObjectId id, xmlNode* cnode ):
            uniset::UniSetObject(id) {}

        virtual ~TestUObject() {};

        // специальные функции для проведения тестирования
        inline uniset::VoidMessagePtr getOneMessage()
        {
            return receiveMessage();
        }

        inline bool mqEmpty()
        {
            return (countMessages() == 0);
        }

    protected:
        TestUObject() {};
};
// -------------------------------------------------------------------------
#endif // TestUObject_H_
// -------------------------------------------------------------------------
