#include <iostream>
#include <memory>
#include "UHttpServer.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// --------------------------------------------------------------------------
class UTestSupplier:
		public UHttp::IHttpRequest
{
	public:
		UTestSupplier(){}
		virtual ~UTestSupplier(){}

		virtual nlohmann::json getData() override
		{
			nlohmann::json j;

			j["test"] = 42;
			return j;
		}
};
// --------------------------------------------------------------------------
class UTestRequestRegistry:
		public UHttp::IHttpRequestRegistry
{
	public:
		UTestRequestRegistry(){}
		virtual ~UTestRequestRegistry(){}


		virtual nlohmann::json getDataByName( const std::string& name ) override
		{
			nlohmann::json j = sup.getData();
			j["name"] = name;
			return j;
		}

	private:
		UTestSupplier sup;
};

// --------------------------------------------------------------------------
int main(int argc, const char** argv)
{
	try
	{
		auto reg = std::make_shared<UTestRequestRegistry>();
		auto ireg = dynamic_pointer_cast<UHttp::IHttpRequestRegistry>(reg);

		auto http = make_shared<UHttp::UHttpServer>(ireg,"localhost", 5555);
		http->log()->level(Debug::ANY);

		http->start();
		pause();
		http->stop();
		return 0;
	}
	catch( const std::exception& e )
	{
		cerr << "(http-test): " << e.what() << endl;
	}
	catch(...)
	{
		cerr << "(http-test): catch(...)" << endl;
	}

	return 1;
}
