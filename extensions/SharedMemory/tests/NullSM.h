// --------------------------------------------------------------------------
#ifndef NullSM_H_
#define NullSM_H_
// --------------------------------------------------------------------------
#include <string>
#include "IONotifyController.h"
// --------------------------------------------------------------------------
class NullSM:
    public uniset::IONotifyController
{
    public:
        NullSM( uniset::ObjectId id, const std::string& datfile );

        virtual ~NullSM();

    protected:

        virtual void logging( uniset::SensorMessage& sm ) override {};

    private:

};
// --------------------------------------------------------------------------
#endif // NullSM_H_
// --------------------------------------------------------------------------
