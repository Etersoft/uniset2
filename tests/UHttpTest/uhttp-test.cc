#ifndef DISABLE_REST_API
#include <iostream>
#include <memory>
#include "UHttpServer.h"
#include <Poco/JSON/Object.h>
#include "ujson.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// --------------------------------------------------------------------------
class UTestSupplier:
		public UHttp::IHttpRequest
{
	public:
		UTestSupplier(){}
		virtual ~UTestSupplier(){}

		virtual Poco::JSON::Object::Ptr  httpGet( const Poco::URI::QueryParameters& params ) override
		{
			Poco::JSON::Object::Ptr j = new Poco::JSON::Object();

			for( const auto& p: params )
				j->set(p.first,p.second);

			j->set("test",42);
			return j;
		}

		virtual Poco::JSON::Object::Ptr httpHelp( const Poco::URI::QueryParameters& p ) override
		{
			uniset::json::help::object myhelp("test");

			uniset::json::help::item cmd1("description for cmd1");
			cmd1.param("p1","description of p1");
			cmd1.param("p2","description of p1");
			cmd1.param("p3","description of p1");
			myhelp.add(cmd1);

			uniset::json::help::item cmd2("description for cmd2");
			cmd2.param("p1","description of p1");
			cmd2.param("p2","description of p1");
			cmd2.param("p3","description of p1");
			myhelp.add(cmd2);

			cmd1.param("p4","description of p4");
			myhelp.add(cmd1);

			return myhelp;
		}

		virtual Poco::JSON::Object::Ptr  httpRequest( const std::string& req, const Poco::URI::QueryParameters& p ) override
		{
			Poco::JSON::Object::Ptr j = new Poco::JSON::Object();
			j->set(req,"OK");
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


		virtual Poco::JSON::Object::Ptr  httpGetByName( const std::string& name, const Poco::URI::QueryParameters& p ) override
		{
			Poco::JSON::Object::Ptr j = sup.httpGet(p);
			j->set("name",name);
			return j;
		}

		virtual Poco::JSON::Array::Ptr httpGetObjectsList( const Poco::URI::QueryParameters& p ) override
		{
			Poco::JSON::Array::Ptr j = new Poco::JSON::Array();
			j->add("TestObject");
			j->add("TestObject2");
			j->add("TestObject3");
			return j;
		}

		virtual Poco::JSON::Object::Ptr httpHelpByName( const std::string& name, const Poco::URI::QueryParameters& p ) override
		{
			return sup.httpHelp(p);
		}

		virtual Poco::JSON::Object::Ptr httpRequestByName( const std::string& name, const std::string& req, const Poco::URI::QueryParameters& p ) override
		{
			return sup.httpRequest(req,p);
		}


	private:
		UTestSupplier sup;
};
// --------------------------------------------------------------------------
int main(int argc, const char** argv)
{
	try
	{
//		Poco::JSON::Object::Ptr j = new Poco::JSON::Object();
//		{
//			Poco::DynamicStruct data;
//			Poco::Dynamic::Array objects;

//			Poco::DynamicStruct object;
//			object["id"] = 4565;
//			object["size"] = 2.64;
//			object["name"] = "Foo";
//			object["active"] = false;
//			objects.push_back(object);

//			data["objects"] = objects;
//			data["count"] = 1;

//			std::string s = data.toString();

//			std::cout << s << std::endl;

//			Poco::DynamicAny result_s = Poco::DynamicAny::parse(s);

//			std::cout << result_s.toString() << std::endl;


//			j->set("test",object);
//		}

//		j->stringify(std::cout);
//		cout << endl;

//		Poco::JSON::Object::Ptr j = new Poco::JSON::Object();
//		j->set("test",23);
//		j->set("test2","sdfsdf");
//		j->set("test3",232.4);

//		Poco::JSON::Object::Ptr j2 = new Poco::JSON::Object();
//		j2->set("rr",23);
//		j2->set("rr2",23);
//		j2->set("rr3",23);

//		Poco::JSON::Array::Ptr j3 = new Poco::JSON::Array();
//		j3->set(1,23);
//		j3->set(2,23);
//		j3->set(3,23);

//		j->set("Object2",j2);
//		j->set("Object3",j3);

//		j->stringify(std::cerr);
//		cerr << endl;
//		return 0;

//		auto j = uniset::json::make_object("key","weweew");
//		j->set("key2","wefwefefr");

//		auto j2 = uniset::json::make_object("key",j);

////		uniset::json j;
////		j["key"] = "werwe";
////		j["key"]["key2"] = "werwe";

//		j2->stringify(cerr);

//		return 0;


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
#else
#include <iostream>
int main(int argc, const char** argv)
{
	std::cerr << "REST API DISABLED!!" << std::endl;
	return 1;
}
#endif // #ifndef DISABLE_REST_API
