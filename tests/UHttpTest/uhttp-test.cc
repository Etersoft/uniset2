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

		virtual nlohmann::json httpGet( const Poco::URI::QueryParameters& params ) override
		{
			nlohmann::json j;

			for( const auto& p: params )
				j[p.first] = p.second;

			j["test"] = 42;
			return j;
		}

		virtual nlohmann::json httpHelp( const Poco::URI::QueryParameters& p ) override
		{
			nlohmann::json j;
			j["test"]["help"] = {
				{"cmd1","help for cmd1"},
				{"cmd2","help for cmd2"}
			};

			return j;
		}

		virtual nlohmann::json httpRequest( const std::string& req, const Poco::URI::QueryParameters& p ) override
		{
			nlohmann::json j;
			j[req] = "OK";
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


		virtual nlohmann::json httpGetByName( const std::string& name, const Poco::URI::QueryParameters& p ) override
		{
			nlohmann::json j = sup.httpGet(p);
			j["name"] = name;
			return j;
		}

		virtual nlohmann::json httpGetObjectsList( const Poco::URI::QueryParameters& p ) override
		{
			nlohmann::json j;
			j.push_back("TestObject");
			j.push_back("TestObject2");
			j.push_back("TestObject3");
			return j;
		}

		virtual nlohmann::json httpHelpByName( const std::string& name, const Poco::URI::QueryParameters& p ) override
		{
			nlohmann::json j;
			j["TestObject"]["help"] = {
				{"cmd1","help for cmd1"},
				{"cmd2","help for cmd2"}
			};

			return j;
		}

		virtual nlohmann::json httpRequestByName( const std::string& name, const std::string& req, const Poco::URI::QueryParameters& p ) override
		{
			nlohmann::json j;
			j[name][req] = "OK";
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
