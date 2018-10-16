// --------------------------------------------------------------------------
#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <getopt.h>
// --------------------------------------------------------------------------
#include "ORepHelpers.h"
#include "ObjectRepository.h"
#include "Exceptions.h"
#include "UniSetObject.h"
#include "UniSetTypes.h"
#include "UniSetManager.h"
#include "MessageType.h"
#include "UInterface.h"
#include "Configuration.h"
#include "ObjectIndex_XML.h"
#include "Debug.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// --------------------------------------------------------------------------
enum Command
{
	StartUp,
	FoldUp,
	Finish,
	Exist,
	Configure,
	LogRotate
};

static struct option longopts[] =
{
	{ "help", no_argument, 0, 'h' },
	{ "confile", required_argument, 0, 'c' },
	{ "create", no_argument, 0, 'b' },
	{ "exist", no_argument, 0, 'e' },
	{ "omap", no_argument, 0, 'o' },
	{ "start", no_argument, 0, 's' },
	{ "finish", no_argument, 0, 'f' },
	{ "foldUp", no_argument, 0, 'u' },
	{ "configure", optional_argument, 0, 'r' },
	{ "logrotate", optional_argument, 0, 'l' },
	{ "info", required_argument, 0, 'i' },
	{ "setValue", required_argument, 0, 'x' },
	{ "getValue", required_argument, 0, 'g' },
	{ "getRawValue", required_argument, 0, 'w' },
	{ "getCalibrate", required_argument, 0, 'y' },
	{ "getTimeChange", required_argument, 0, 't' },
	{ "oinfo", required_argument, 0, 'p' },
	{ "apiRequest", required_argument, 0, 'a' },
	{ "verbose", no_argument, 0, 'v' },
	{ "quiet", no_argument, 0, 'q' },
	{ "csv", required_argument, 0, 'z' },
	{ NULL, 0, 0, 0 }
};

string conffile("configure.xml");

// --------------------------------------------------------------------------
static bool commandToAll( const string& section, std::shared_ptr<ObjectRepository>& rep, Command cmd );
static void createSections(const std::shared_ptr<Configuration>& c );
static void errDoNotResolve( const std::string& oname );
static char* checkArg( int ind, int argc, char* argv[] );
// --------------------------------------------------------------------------
int omap();
int configure( const string& args, UInterface& ui );
int logRotate( const string& args, UInterface& ui );
int setValue( const string& args, UInterface& ui );
int getValue( const string& args, UInterface& ui );
int getRawValue( const string& args, UInterface& ui );
int getTimeChange( const string& args, UInterface& ui );
int getState( const string& args, UInterface& ui );
int getCalibrate( const string& args, UInterface& ui );
int oinfo(const string& args, UInterface& ui , const string&  userparam );
int apiRequest( const string& args, UInterface& ui, const string& query );
// --------------------------------------------------------------------------
static void print_help(int width, const string& cmd, const string& help, const string& tab = " " )
{
	uniset::ios_fmt_restorer ifs(cout);
	cout.setf(ios::left, ios::adjustfield);
	cout << tab << setw(width) << cmd << " - " << help;
}
// --------------------------------------------------------------------------
static void short_usage()
{
	cout << "Usage: uniset-admin [--confile configure.xml] --command [arg]    \n for detailed information arg --help" << endl;
}

// --------------------------------------------------------------------------
static void usage()
{
	cout << "\nUsage: \n\tuniset-admin [--confile configure.xml] --command [arg]\n";
	cout << "commands list:\n";
	cout << "-----------------------------------------\n";
	print_help(24, "-с|--confile file.xml ", "Используемый конфигурационный файл\n");
	cout << endl;
	print_help(24, "-b|--create ", "Создание репозитория\n");
	print_help(24, "-e|--exist ", "Вызов функции exist() показывающей какие объекты зарегистрированы и доступны.\n");
	print_help(24, "-o|--omap ", "Вывод на экран списка объектов с идентификаторами.\n");
	print_help(24, "-s|--start ", "Посылка SystemMessage::StartUp всем объектам (процессам)\n");
	print_help(24, "-u|--foldUp ", "Посылка SystemMessage::FoldUp всем объектам (процессам)\n");
	print_help(24, "-f|--finish ", "Посылка SystemMessage::Finish всем объектам (процессам)\n");
	print_help(24, "-h|--help  ", "Вывести это сообщение.\n");
	cout << endl;
	print_help(36, "-r|--configure [FullObjName] ", "Посылка SystemMessage::ReConfiguration всем объектам (процессам) или заданному по имени (FullObjName).\n");
	print_help(36, "-l|--logrotate [FullObjName] ", "Посылка SystemMessage::LogRotate всем объектам (процессам) или заданному по имени (FullObjName).\n");
	print_help(36, "-p|--oinfo id1@node1,id2@node2,id3,... [userparam]", "Получить информацию об объектах (SimpleInfo). \n userparam - необязательный параметр передаваемый в getInfo() каждому объекту\n");
	print_help(36, "", "userparam - необязательный параметр передаваемый в getInfo() каждому объекту\n");
	cout << endl;
	print_help(36, "-a|--apiRequest id1@node1,id2@node2,id3,... query", "Вызов REST API для каждого объекта\n");
	print_help(36, "", "query - Запрос вида: /api/VERSION/query[?param1&param2...]\n");
	cout << endl;
	print_help(48, "-x|--setValue id1@node1=val,id2@node2=val2,id3=val3,.. ", "Выставить значения датчиков\n");
	print_help(36, "-g|--getValue id1@node1,id2@node2,id3,id4 ", "Получить значения датчиков.\n");
	cout << endl;
	print_help(36, "-w|--getRawValue id1@node1,id2@node2,id3,.. ", "Получить 'сырое' значение.\n");
	print_help(36, "-y|--getCalibrate id1@node1,id2@node2,id3,.. ", "Получить параметры калибровки.\n");
	print_help(36, "-t|--getTimeChange id1@node1,id2@node2,id3,.. ", "Получить время последнего изменения.\n");
	print_help(36, "-v|--verbose", "Подробный вывод логов.\n");
	print_help(36, "-q|--quiet", "Выводит только результат.\n");
	print_help(36, "-z|--csv", "Вывести результат (getValue) в виде val1,val2,val3...\n");
	cout << endl;
}

// --------------------------------------------------------------------------------------
/*!
    \todo Оптимизировать commandToAll, т.к. сейчас НА КАЖДОМ ШАГЕ цикла
        создаётся сообщение и происходит преобразование в TransportMessage.
        TransportMessage можно создать один раз до цикла.
*/
// --------------------------------------------------------------------------------------
static bool verb = false;
static bool quiet = false;
static bool csv = false;

int main(int argc, char** argv)
{
	//	std::ios::sync_with_stdio(false);

	try
	{
		int optindex = 0;
		int opt = 0;

		while(1)
		{
			opt = getopt_long(argc, argv, "hc:beosfur:l:i::x:g:w:y:p:vqz:a:", longopts, &optindex);

			if( opt == -1 )
				break;

			switch (opt) //разбираем параметры
			{
				case 'h':    //--help
					usage();
					return 0;

				case 'v':
					verb = true;
					break;

				case 'q':
					quiet = true;
					break;

				case 'c':    //--confile
					conffile = optarg;
					break;

				case 'o':    //--omap
				{
					uniset_init(argc, argv, conffile);
					return omap();
				}
				break;

				case 'b':    //--create
				{
					auto conf = uniset_init(argc, argv, conffile);
					createSections(conf);
				}

				return 0;

				case 'x':    //--setValue
				{
					auto conf = uniset_init(argc, argv, conffile);
					UInterface ui(conf);
					ui.initBackId(uniset::AdminID);
					string name = ( optarg ) ? optarg : "";
					return setValue(name, ui);
				}
				break;

				case 'g':    //--getValue
				case 'z':    //--csv
				{
					if( opt == 'z' )
						csv = true;

					//                    cout<<"(main):received option --getValue='"<<optarg<<"'"<<endl;
					auto conf = uniset_init(argc, argv, conffile);
					UInterface ui(conf);
					ui.initBackId(uniset::AdminID);
					string name = ( optarg ) ? optarg : "";
					return getValue(name, ui);
				}
				break;

				case 'w':    //--getRawValue
				{
					//                cout<<"(main):received option --getRawValue='"<<optarg<<"'"<<endl;
					auto conf = uniset_init(argc, argv, conffile);
					UInterface ui(conf);
					ui.initBackId(uniset::AdminID);
					string name = ( optarg ) ? optarg : "";
					return getRawValue(name, ui);
				}
				break;

				case 't':    //--getTimeChange
				{
					auto conf = uniset_init(argc, argv, conffile);
					UInterface ui(conf);
					ui.initBackId(uniset::AdminID);
					string name = ( optarg ) ? optarg : "";
					return getTimeChange(name, ui);
				}
				break;

				case 'p':    //--oinfo
				{
					//                    cout<<"(main):received option --oinfo='"<<optarg<<"'"<<endl;
					auto conf = uniset_init(argc, argv, conffile);
					UInterface ui(conf);
					ui.initBackId(uniset::AdminID);

					std::string userparam = { "" };

					// смотрим второй параметр
					if( checkArg(optind, argc, argv) )
						userparam = string(argv[optind]);

					return oinfo(optarg, ui, userparam);
				}
				break;

				case 'a':    //--apiRequest
				{
					// смотрим второй параметр
					if( checkArg(optind, argc, argv) == 0 )
					{
						if( !quiet )
							cerr << "admin(apiRequest): Unknown 'query'. Use: id,name,name2@nodeX /api/vesion/query.." << endl;

						return 1;
					}

					auto conf = uniset_init(argc, argv, conffile);
					UInterface ui(conf);
					ui.initBackId(uniset::AdminID);

					std::string query = string(argv[optind]);

					return apiRequest(optarg, ui, query);
				}
				break;

				case 'e':    //--exist
				{
					//                    cout<<"(main):received option --exist"<<endl;
					auto conf = uniset_init(argc, argv, conffile);
					UInterface ui(conf);
					ui.initBackId(uniset::AdminID);

					verb = true;
					Command cmd = Exist;
					auto rep = make_shared<ObjectRepository>(conf);
					commandToAll(conf->getServicesSection(), rep, (Command)cmd);
					commandToAll(conf->getControllersSection(), rep, (Command)cmd);
					commandToAll(conf->getObjectsSection(), rep, (Command)cmd);
				}

				return 0;

				case 's':    //--start
				{
					//                    cout<<"(main):received option --start"<<endl;
					auto conf = uniset_init(argc, argv, conffile);
					UInterface ui(conf);
					ui.initBackId(uniset::AdminID);

					Command cmd = StartUp;
					auto rep = make_shared<ObjectRepository>(conf);
					commandToAll(conf->getServicesSection(), rep, (Command)cmd);
					commandToAll(conf->getControllersSection(), rep, (Command)cmd);
					commandToAll(conf->getObjectsSection(), rep, (Command)cmd);
				}

				return 0;

				case 'r':    //--configure
				{
					auto conf = uniset_init(argc, argv, conffile);
					UInterface ui(conf);
					ui.initBackId(uniset::AdminID);
					string name = ( optarg ) ? optarg : "";
					return configure(name, ui);
				}
				break;

				case 'f':    //--finish
				{
					//                    cout<<"(main):received option --finish"<<endl;
					auto conf = uniset_init(argc, argv, conffile);
					UInterface ui(conf);
					ui.initBackId(uniset::AdminID);

					Command cmd = Finish;
					auto rep = make_shared<ObjectRepository>(conf);
					commandToAll(conf->getServicesSection(), rep, (Command)cmd);
					commandToAll(conf->getControllersSection(), rep, (Command)cmd);
					commandToAll(conf->getObjectsSection(), rep, (Command)cmd);

					if( verb )
						cout << "(finish): done" << endl;
				}

				return 0;

				case 'l':    //--logrotate
				{
					auto conf = uniset_init(argc, argv, conffile);
					UInterface ui(conf);
					ui.initBackId(uniset::AdminID);

					string name = ( optarg ) ? optarg : "";
					return logRotate(name, ui);
				}
				break;

				case 'y':    //--getCalibrate
				{
					//                    cout<<"(main):received option --getCalibrate='"<<optarg<<"'"<<endl;
					auto conf = uniset_init(argc, argv, conffile);
					UInterface ui(conf);
					ui.initBackId(uniset::AdminID);
					string name = ( optarg ) ? optarg : "";
					return getCalibrate(name, ui);
				}
				break;

				case 'u':    //--foldUp
				{
					//                    cout<<"(main):received option --foldUp"<<endl;
					auto conf = uniset_init(argc, argv, conffile);
					UInterface ui(conf);
					ui.initBackId(uniset::AdminID);

					Command cmd = FoldUp;
					auto rep = make_shared<ObjectRepository>(conf);
					commandToAll(conf->getServicesSection(), rep, (Command)cmd);
					commandToAll(conf->getControllersSection(), rep, (Command)cmd);
					commandToAll(conf->getObjectsSection(), rep, (Command)cmd);
					//                    cout<<"(foldUp): done"<<endl;
				}

				return 0;

				case '?':
				default:
				{
					short_usage();
					return 1;
				}
			}
		}

		return 0;
	}
	catch( const uniset::Exception& ex )
	{
		if( !quiet )
			cout << "admin(main): " << ex << endl;
	}
	catch( const CORBA::SystemException& ex )
	{
		if( !quiet )
			cerr << "поймали CORBA::SystemException: " << ex.NP_minorString() << endl;
	}
	catch( const CORBA::Exception& )
	{
		if( !quiet )
			cerr << "поймали CORBA::Exception." << endl;
	}
	catch( const omniORB::fatalException& fe )
	{
		if( !quiet )
		{
			cerr << "поймали omniORB::fatalException: " << endl;
			cerr << "  file: " << fe.file() << endl;
			cerr << "  line: " << fe.line() << endl;
			cerr << "  mesg: " << fe.errmsg() << endl;
		}
	}
	catch( std::exception& ex )
	{
		if( !quiet )
			cerr << "exception: " << ex.what() << endl;
	}
	catch(...)
	{
		if( !quiet )
			cerr << "Unknown exception.." << endl;
	}

	return 1;
}

// ==============================================================================================
static bool commandToAll(const string& section, std::shared_ptr<ObjectRepository>& rep, Command cmd)
{
	if( verb )
		cout << "\n||=======********  " << section << "  ********=========||\n" << endl;

	uniset::ios_fmt_restorer ifs(cout);

	cout.setf(ios::left, ios::adjustfield);

	try
	{
		ListObjectName olist;
		rep->list(section, &olist);

		if( olist.empty() )
		{
			if( verb )
				cout << "пусто!" << endl;

			return false;
		}

		UniSetManager_i_var proc;
		UniSetObject_i_var obj;
		string fullName;

		for( const auto& oname : olist )
		{
			fullName = section + "/" + oname;

			try
			{
				uniset::ObjectVar o = rep->resolve(fullName);
				obj = UniSetObject_i::_narrow(o);

				switch( cmd )
				{
					case StartUp:
					{
						if( CORBA::is_nil(obj) )
						{
							errDoNotResolve(oname);
							break;
						}

						SystemMessage msg(SystemMessage::StartUp);
						obj->push( Message::transport(msg) );

						if( verb )
							cout << setw(55) << oname << "   <--- start OK" <<   endl;
					}
					break;

					case FoldUp:
					{
						if(CORBA::is_nil(obj))
						{
							errDoNotResolve(oname);
							break;
						}

						SystemMessage msg(SystemMessage::FoldUp);
						obj->push( Message::transport(msg) );

						if( verb )
							cout << setw(55) << oname << "   <--- foldUp OK" <<   endl;
					}
					break;

					case Finish:
					{
						if(CORBA::is_nil(obj))
						{
							errDoNotResolve(oname);
							break;
						}

						SystemMessage msg(SystemMessage::Finish);
						obj->push( Message::transport(msg) );

						if( verb )
							cout << setw(55) << oname << "   <--- finish OK" <<   endl;
					}
					break;

					case Exist:
					{
						if( obj->exist() )
							cout << "(" << setw(6) << obj->getId() << ")" << setw(55) << oname << "   <--- exist ok\n";
						else
							cout << "(" << setw(6) << obj->getId() << ")" << setw(55) << oname << "   <--- exist NOT OK\n";
					}
					break;

					case Configure:
					{
						SystemMessage sm(SystemMessage::ReConfiguration);
						obj->push(sm.transport_msg());

						if( verb )
							cout << setw(55) << oname << "   <--- configure ok\n";
					}
					break;

					case LogRotate:
					{
						SystemMessage msg(SystemMessage::LogRotate);
						obj->push( Message::transport(msg) );

						if( verb )
							cout << setw(55) << oname << "   <--- logrotate ok\n";

						break;
					}

					default:
					{
						if( !quiet )
							cout << "неизвестная команда -" << cmd << endl;

						return false;
					}
				}
			}
			catch( const uniset::Exception& ex )
			{
				if( !quiet )
					cerr << setw(55) << oname << "   <--- " << ex << endl;
			}
			catch( const CORBA::SystemException& ex )
			{
				if( !quiet )
					cerr << setw(55) << oname  << "   <--- недоступен!!(CORBA::SystemException): " << ex.NP_minorString() << endl;
			}
			catch( const std::exception& ex )
			{
				if( !quiet )
					cerr << "std::exception: " << ex.what() << endl;
			}
		}
	}
	catch( const ORepFailed& ex )
	{
		if( !quiet )
			cerr << "..ORepFailed.." << endl;

		return false;
	}

	return true;
}

// ==============================================================================================
static void createSections( const std::shared_ptr<uniset::Configuration>& rconf )
{
	ObjectRepository repf(rconf);

	repf.createRootSection(rconf->getRootSection());
	repf.createRootSection(rconf->getSensorsSection());
	repf.createRootSection(rconf->getObjectsSection());
	repf.createRootSection(rconf->getControllersSection());
	repf.createRootSection(rconf->getServicesSection());

	if( verb )
		cout << "(create): created" << endl;
}

// ==============================================================================================
int omap()
{
	uniset::ios_fmt_restorer ifs(cout);

	try
	{
		cout.setf(ios::left, ios::adjustfield);
		cout << "========================== ObjectsMap  =================================\n";
		uniset_conf()->oind->printMap(cout);
		cout << "==========================================================================\n";
	}
	catch( const uniset::Exception& ex )
	{
		if( !quiet )
			cerr << " configuration init failed: " << ex << endl;

		return 1;
	}
	catch( const std::exception& ex )
	{
		if( !quiet )
			cerr << "std::exception: " << ex.what() << endl;

		return 1;
	}

	return 0;
}

// --------------------------------------------------------------------------------------
int setValue( const string& args, UInterface& ui )
{
	int err = 0;
	auto conf = ui.getConf();
	auto sl = uniset::getSInfoList(args, conf);

	if( verb )
		cout << "====== setValue ======" << endl;

	for( auto && it : sl )
	{
		try
		{
			UniversalIO::IOType t = conf->getIOType(it.si.id);

			if( verb )
			{
				cout << "  value: " << it.val << endl;
				cout << "   name: (" << it.si.id << ") " << it.fname << endl;
				cout << " iotype: " << t << endl;
				cout << "   text: " << conf->oind->getTextName(it.si.id) << "\n\n";
			}

			if( it.si.node == DefaultObjectId )
				it.si.node = conf->getLocalNode();

			switch(t)
			{
				case UniversalIO::DI:
				case UniversalIO::DO:
				case UniversalIO::AI:
				case UniversalIO::AO:
					ui.setValue(it.si.id, it.val, it.si.node, AdminID);
					break;

				default:
					if( !quiet )
						cerr << "FAILED: Unknown 'iotype' for " << it.fname << endl;

					err = 1;
					break;
			}
		}
		catch( const uniset::Exception& ex )
		{
			if( !quiet )
				cerr << "(setValue): " << ex << endl;;

			err = 1;
		}
		catch( const std::exception& ex )
		{
			if( !quiet )
				cerr << "std::exception: " << ex.what() << endl;

			err = 1;
		}
	}

	return err;
}

// --------------------------------------------------------------------------------------
int getValue( const string& args, UInterface& ui )
{
	int err = 0;

	auto conf = ui.getConf();
	auto sl = uniset::getSInfoList( args, conf );

	if( csv )
		quiet = true;

	if( !quiet )
		cout << "====== getValue ======" << endl;

	size_t num = 0;

	for( auto && it : sl )
	{
		try
		{
			UniversalIO::IOType t = conf->getIOType(it.si.id);

			if( !quiet )
			{
				cout << "   name: (" << it.si.id << ") " << it.fname << endl;
				cout << "   iotype: " << t << endl;
				cout << "   text: " << conf->oind->getTextName(it.si.id) << "\n\n";
			}

			if( it.si.node == DefaultObjectId )
				it.si.node = conf->getLocalNode();

			switch(t)
			{
				case UniversalIO::DO:
				case UniversalIO::DI:
				case UniversalIO::AO:
				case UniversalIO::AI:
					if( !quiet )
						cout << "  value: " << ui.getValue(it.si.id, it.si.node) << endl;
					else
					{
						if( csv )
						{
							// т.к. может сработать исключение, а нам надо вывести ','
							// до числа, то сперва получаем val
							long val = ui.getValue(it.si.id, it.si.node);

							if( csv && num++ > 0 )
								cout << ",";

							cout << val;
						}
						else
							cout << ui.getValue(it.si.id, it.si.node);
					}

					break;

				default:
					if( !quiet )
						cerr << "FAILED: Unknown 'iotype' for " << it.fname << endl;

					err = 1;
					break;
			}
		}
		catch( const uniset::Exception& ex )
		{
			if( !quiet )
				cerr << "(getValue): " << ex << endl;

			err = 1;
		}
		catch( const std::exception& ex )
		{
			if( !quiet )
				cerr << "std::exception: " << ex.what() << endl;

			err = 1;
		}
	}

	return err;
}
// --------------------------------------------------------------------------------------
int getCalibrate( const std::string& args, UInterface& ui )
{
	int err = 0;
	auto conf = ui.getConf();
	auto sl = uniset::getSInfoList( args, conf );

	if( !quiet )
		cout << "====== getCalibrate ======" << endl;

	for( auto && it : sl )
	{
		if( it.si.node == DefaultObjectId )
			it.si.node = conf->getLocalNode();

		try
		{
			if( !quiet )
			{
				cout << "      name: (" << it.si.id << ") " << it.fname << endl;
				cout << "      text: " << conf->oind->getTextName(it.si.id) << "\n";
				cout << "калибровка: ";
			}

			IOController_i::CalibrateInfo ci = ui.getCalibrateInfo(it.si);

			if( !quiet )
				cout << ci << endl;
			else
				cout << ci;
		}
		catch( const uniset::Exception& ex )
		{
			if( !quiet )
				cerr << "(getCalibrate): " << ex << endl;;

			err = 1;
		}
		catch( const std::exception& ex )
		{
			if( !quiet )
				cerr << "std::exception: " << ex.what() << endl;

			err = 1;
		}
	}

	return err;
}

// --------------------------------------------------------------------------------------
int getRawValue( const std::string& args, UInterface& ui )
{
	int err = 0;
	auto conf = ui.getConf();
	auto sl = uniset::getSInfoList( args, conf );

	if( !quiet )
		cout << "====== getRawValue ======" << endl;

	for( auto && it : sl )
	{
		if( it.si.node == DefaultObjectId )
			it.si.node = conf->getLocalNode();

		try
		{
			if( !quiet )
			{
				cout << "   name: (" << it.si.id << ") " << it.fname << endl;
				cout << "   text: " << conf->oind->getTextName(it.si.id) << "\n\n";
				cout << "  value: " << ui.getRawValue(it.si) << endl;
			}
			else
				cout << ui.getRawValue(it.si);
		}
		catch( const uniset::Exception& ex )
		{
			if( !quiet )
				cerr << "(getRawValue): " << ex << endl;;

			err = 1;
		}
		catch( const std::exception& ex )
		{
			if( !quiet )
				cerr << "std::exception: " << ex.what() << endl;

			err = 1;
		}
	}

	return err;
}

// --------------------------------------------------------------------------------------
int getTimeChange( const std::string& args, UInterface& ui )
{
	int err = 0;
	auto conf = ui.getConf();
	auto sl = uniset::getSInfoList( args, conf );

	if( !quiet )
		cout << "====== getChangedTime ======" << endl;

	for( auto && it : sl )
	{
		if( it.si.node == DefaultObjectId )
			it.si.node = conf->getLocalNode();

		try
		{
			if( !quiet )
			{
				cout << "   name: (" << it.si.id << ") " << it.fname << endl;
				cout << "   text: " << conf->oind->getTextName(it.si.id) << "\n\n";
				cout << ui.getTimeChange(it.si.id, it.si.node) << endl;
			}
			else
				cout << ui.getTimeChange(it.si.id, it.si.node);
		}
		catch( const uniset::Exception& ex )
		{
			if( !quiet )
				cerr << "(getChangedTime): " << ex << endl;;

			err = 1;
		}
		catch( const CORBA::SystemException& ex )
		{
			if( !quiet )
				cerr << "CORBA::SystemException: " << ex.NP_minorString() << endl;

			err = 1;
		}
		catch( const CORBA::Exception& )
		{
			if( !quiet )
				cerr << "CORBA::Exception." << endl;

			err = 1;
		}
		catch( const omniORB::fatalException& fe )
		{
			if( !quiet )
			{
				cerr << "omniORB::fatalException: " << endl;
				cerr << "  file: " << fe.file() << endl;
				cerr << "  line: " << fe.line() << endl;
				cerr << "  mesg: " << fe.errmsg() << endl;
			}

			err = 1;
		}
		catch( const std::exception& ex )
		{
			if( !quiet )
				cerr << "std::exception: " << ex.what() << endl;

			err = 1;
		}
		catch(...)
		{
			if( !quiet )
				cerr << "Unknown exception.." << endl;

			err = 1;
		}
	}

	return err;
}

// --------------------------------------------------------------------------------------
int logRotate( const string& arg, UInterface& ui )
{
	auto conf = ui.getConf();

	// посылка всем
	if( arg.empty() || arg[0] == '-' )
	{
		auto rep = make_shared<ObjectRepository>(conf);
		commandToAll(conf->getServicesSection(), rep, (Command)LogRotate);
		commandToAll(conf->getControllersSection(), rep, (Command)LogRotate);
		commandToAll(conf->getObjectsSection(), rep, (Command)LogRotate);
	}
	else // посылка определённому объекту
	{
		uniset::ObjectId id = conf->getObjectID(arg);

		if( id == DefaultObjectId )
			id = conf->getControllerID(arg);

		if( id == DefaultObjectId )
			id = conf->getServiceID(arg);

		if( id == DefaultObjectId )
		{
			if( !quiet )
				cout << "(logrotate): not found ID for name='" << arg << "'" << endl;

			return 1;
		}


		TransportMessage tm( SystemMessage(SystemMessage::LogRotate).transport_msg() );
		ui.send(id, tm);

		if( verb )
			cout << "\nSend 'LogRotate' to " << arg << " OK.\n";
	}

	return 0;
}

// --------------------------------------------------------------------------------------
int configure( const string& arg, UInterface& ui )
{
	auto conf = ui.getConf();

	// посылка всем
	if( arg.empty() || arg[0] == '-' )
	{
		auto rep = make_shared<ObjectRepository>(conf);
		commandToAll(conf->getServicesSection(), rep, (Command)Configure);
		commandToAll(conf->getControllersSection(), rep, (Command)Configure);
		commandToAll(conf->getObjectsSection(), rep, (Command)Configure);
	}
	else // посылка определённому объекту
	{
		uniset::ObjectId id = conf->getObjectID(arg);

		if( id == DefaultObjectId )
			id = conf->getControllerID(arg);

		if( id == DefaultObjectId )
			id = conf->getServiceID(arg);

		if( id == DefaultObjectId )
		{
			if( !quiet )
				cout << "(configure): name='" << arg << "' не найдено!!!\n";

			return 1;
		}

		TransportMessage tm( SystemMessage(SystemMessage::ReConfiguration).transport_msg() );
		ui.send(id, tm);

		if( verb )
			cout << "\nSend 'ReConfigure' to " << arg << " OK.\n";
	}

	return 0;
}

// --------------------------------------------------------------------------------------
int oinfo(const string& args, UInterface& ui, const string& userparam )
{
	auto conf = uniset_conf();
	auto sl = uniset::getObjectsList( args, conf );

	for( auto && it : sl )
	{
		if( it.node == DefaultObjectId )
			it.node = conf->getLocalNode();

		try
		{
			cout << ui.getObjectInfo(it.id, userparam, it.node) << endl;
		}
		catch( const std::exception& ex )
		{
			if( !quiet )
				cerr << "std::exception: " << ex.what() << endl;
		}
		catch(...)
		{
			if( !quiet )
				cerr << "Unknown exception.." << endl;
		}

		cout << endl << endl;
	}

	return 0;
}
// --------------------------------------------------------------------------------------
int apiRequest( const string& args, UInterface& ui, const string& query )
{
	auto conf = uniset_conf();
	auto sl = uniset::getObjectsList( args, conf );

	//	if( verb )
	//		cout << "apiRequest: query: " << query << endl;

	for( auto && it : sl )
	{
		if( it.node == DefaultObjectId )
			it.node = conf->getLocalNode();

		try
		{
			cout << ui.apiRequest(it.id, query, it.node) << endl;
		}
		catch( const std::exception& ex )
		{
			if( !quiet )
				cerr << "std::exception: " << ex.what() << endl;
		}
		catch(...)
		{
			if( !quiet )
				cerr << "Unknown exception.." << endl;
		}

		cout << endl << endl;
	}

	return 0;
}

// --------------------------------------------------------------------------------------
void errDoNotResolve( const std::string& oname )
{
	if( verb )
		cerr << oname << ": resolve failed.." << endl;
}
// --------------------------------------------------------------------------------------
char* checkArg( int i, int argc, char* argv[] )
{
	if( i < argc && (argv[i])[0] != '-' )
		return argv[i];

	return 0;
}
// --------------------------------------------------------------------------------------
