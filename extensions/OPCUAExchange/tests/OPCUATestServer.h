#ifndef OPCUATestServer_H_
#define OPCUATestServer_H_
// -------------------------------------------------------------------------
#include <string>
#include <atomic>
#include <ostream>
#include "open62541pp/open62541pp.h"
#include "ThreadCreator.h"
// -------------------------------------------------------------------------
/*! Реализация OPCUATestServer для тестирования */
class OPCUATestServer
{
    public:
        OPCUATestServer( const std::string& addr, uint16_t port = 4840 );
        ~OPCUATestServer();

        inline void setVerbose( bool state )
        {
            verbose = state;
        }

        void start();
        void stop();
        bool isRunning();
        void setI32( const std::string& varname, int32_t val );
        void setBool( const std::string& varname, bool set );
        void setI32( int num, int32_t val );
        void setF32( const std::string& varname, float val );

        void setX( int num, int32_t val, opcua::Type type );
        int32_t getX( int num, opcua::Type type );

        int32_t getI32( int num );
        int32_t getI32( const std::string& name );
        float getF32( const std::string& name );
        bool getBool( const std::string& name );

    protected:
        void work();

        struct IONode
        {
            opcua::Node node;
            IONode( const opcua::Node& n ):  node(n) {};
        };

        std::unique_ptr<opcua::Server> server;
        std::unique_ptr<IONode> ioNode;
        std::string addr;
        bool verbose;

        std::unordered_map<int, std::unique_ptr<IONode>> imap;
        std::unordered_map<std::string, std::unique_ptr<IONode>> smap;
        std::shared_ptr< uniset::ThreadCreator<OPCUATestServer> > serverThread;

    private:
        bool disabled;
        std::string myname;
};
// -------------------------------------------------------------------------
#endif // OPCUATestServer_H_
// -------------------------------------------------------------------------
