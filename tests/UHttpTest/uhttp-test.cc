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

		virtual nlohmann::json getData( const Poco::URI::QueryParameters& params ) override
		{
			nlohmann::json j;

			for( const auto& p: params )
				j[p.first] = p.second;

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


		virtual nlohmann::json getDataByName( const std::string& name, const Poco::URI::QueryParameters& p ) override
		{
			nlohmann::json j = sup.getData(p);
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

		cout << "start http test server localhost:5555" << endl; 

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
