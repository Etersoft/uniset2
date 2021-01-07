#ifndef _TestObject_H_
#define _TestObject_H_
// -----------------------------------------------------------------------------
#include "UObject_SK.h"
// -----------------------------------------------------------------------------
class TestObject:
    public UObject_SK
{
    public:
        TestObject( uniset::ObjectId objId, xmlNode* cnode );
        virtual ~TestObject();


    protected:
        TestObject();
};
// -----------------------------------------------------------------------------
#endif // _TestObject_H_
// -----------------------------------------------------------------------------
