// --------------------------------------------------------------------------
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <getopt.h>
// --------------------------------------------------------------------------
#include "ORepHelpers.h"
#include "ObjectRepository.h"
#include "ObjectRepositoryFactory.h"
#include "Exceptions.h"
#include "UniSetObject.h"
#include "UniSetTypes.h"
#include "ObjectsManager.h"
#include "MessageType.h"
#include "Configuration.h"
#include "ObjectIndex_XML.h"
#include "Debug.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
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

static struct option longopts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "confile", required_argument, 0, 'c' },
	{ "create", no_argument, 0, 'b' },
	{ "exist", no_argument, 0, 'e' },
	{ "omap", no_argument, 0, 'o' },
	{ "msgmap", no_argument, 0, 'm' },
	{ "start", no_argument, 0, 's' },
	{ "finish", no_argument, 0, 'f' },
	{ "foldUp", no_argument, 0, 'u' },
	{ "configure", required_argument, 0, 'r' },
	{ "logrotate", required_argument, 0, 'l' },
	{ "info", required_argument, 0, 'i' },
	{ "setValue", required_argument, 0, 'x' },
	{ "getValue", required_argument, 0, 'g' },
	{ "getRawValue", required_argument, 0, 'w' },
	{ "getCalibrate", required_argument, 0, 'y' },
	{ "oinfo", required_argument, 0, 'p' },
	{ NULL, 0, 0, 0 }
};

string conffile("configure.xml");

// --------------------------------------------------------------------------
static bool commandToAll(const string section, ObjectRepository *rep, Command cmd);
static void createSections(UniSetTypes::Configuration* c);
// --------------------------------------------------------------------------
int omap();
int msgmap();
int configure( const string args, UniversalInterface &ui );
int logRotate( const string args, UniversalInterface &ui );
int setValue( const string args, UniversalInterface &ui, Configuration* conf = UniSetTypes::conf );
int getValue( const string args, UniversalInterface &ui, Configuration* conf = UniSetTypes::conf );
int getRawValue( const string args, UniversalInterface &ui );
int getState( const string args, UniversalInterface &ui );
int getCalibrate( const string args, UniversalInterface &ui );
int oinfo( const string args, UniversalInterface &ui );
// --------------------------------------------------------------------------
static void print_help(int width, const string cmd, const string help, const string tab=" " )
{
	// чтобы не менять параметры основного потока
	// создаём свой stream...
	ostringstream info;
	info.setf(ios::left, ios::adjustfield);
	info << tab << setw(width) << cmd << " - " << help;
	cout << info.str();
}
// --------------------------------------------------------------------------
static void short_usage()
{
	cout << "Usage: uniset-admin [--confile configure.xml] --command [arg]	\n for detailed information arg --help" << endl;
}

// --------------------------------------------------------------------------
static void usage()
{
	cout << "\nUsage: \n\tuniset-admin [--confile configure.xml] --command [arg]\n";
	cout << "commands list:\n";
	cout << "-----------------------------------------\n";
	print_help(24, "-с|--confile file.xml ","Используемый конфигурационный файл\n");	
	cout << endl;
	print_help(24, "-b|--create ","Создание репозитория\n");
	print_help(24, "-e|--exist ","Вызов функции exist() показывающей какие объекты зарегистрированы и доступны.\n");
	print_help(24, "-o|--omap ","Вывод на экран списка объектов с идентификаторами.\n");
	print_help(24, "-m|--msgmap ","Вывод на экран списка сообщений с идентификаторами.\n");
	print_help(24, "-s|--start ","Посылка SystemMessage::StartUp всем объектам (процессам)\n");
	print_help(24, "-u|--foldUp ","Посылка SystemMessage::FoldUp всем объектам (процессам)\n");
	print_help(24, "-f|--finish ","Посылка SystemMessage::Finish всем объектам (процессам)\n");
	print_help(24, "-h|--help  ","Вывести это сообщение.\n");	
	cout << endl;
	print_help(36, "-r|--configure [FullObjName] ","Посылка SystemMessage::ReConfiguration всем объектам (процессам) или заданному по имени (FullObjName).\n");
	print_help(36, "-l|--logrotate [FullObjName] ","Посылка SystemMessage::LogRotate всем объектам (процессам) или заданному по имени (FullObjName).\n");
	print_help(36, "-p|--oinfo OID ","Получить информацию об объекте (SimpleInfo).\n");	
	cout << endl;
	print_help(48, "-x|--setValue id1@node1=val,id2@node2=val2,id3=val3,.. ","Выставить значения датчиков\n");
	print_help(36, "-g|--getValue id1@node1,id2@node2,id3,id4 ","Получить значения датчиков.\n");
	cout << endl;
	print_help(36, "-w|--getRawValue id1@node1=val,id2@node2=val2,id3=val3,.. ","Получить 'сырое' значение.\n");
	print_help(36, "-y|--getCalibrate id1@node1=val,id2@node2=val2,id3=val3,.. ","Получить параметры калибровки.\n");
	cout << endl;
}

// --------------------------------------------------------------------------------------
/*! 
	\todo Сделать по умолчанию режим silent и ключ --verbose.
	\todo Оптимизировать commandToAll, т.к. сейчас НА КАЖДОМ ШАГЕ цикла 
		создаётся сообщение и происходит преобразование в TransportMessage. 
		TransportMessage можно создать один раз до цикла.
*/
// --------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
	try
	{
		int optindex = 0;
		char opt = 0;

		while( (opt = getopt_long(argc, argv, "hc:beomsfur:l:i:x:g:w:y:p:",longopts,&optindex)) != -1 )
		{
			switch (opt) //разбираем параметры
			{
				case 'h':	//--help
					usage();
				return 0;

				case 'c':	//--confile
					conffile = optarg;
				break;
			
				case 'o':	//--omap
				{
					uniset_init(argc,argv,conffile);
					return omap();
				}
				break;

				case 'b':	//--create
				{
					uniset_init(argc,argv,conffile);
					createSections(conf);
				}
				return 0;

				case 'm':	//--msgmap
				{
					uniset_init(argc,argv,conffile);
					return msgmap();
				}
				break;

				case 'x':	//--setValue
				{
					uniset_init(argc,argv,conffile);
					UniversalInterface ui(conf);
					return setValue(optarg,ui);
				}
				break;

				case 'g':	//--getValue
				{
//					cout<<"(main):received option --getValue='"<<optarg<<"'"<<endl;
					uniset_init(argc,argv,conffile);
					UniversalInterface ui(conf);
					return getValue(optarg,ui);
				}
				break;

				case 'w':	//--getRawValue
				{
	//				cout<<"(main):received option --getRawValue='"<<optarg<<"'"<<endl;
					uniset_init(argc,argv,conffile);
					UniversalInterface ui(conf);
					return getRawValue(optarg,ui);
				}
				break;

				case 'p':	//--oinfo
				{
//					cout<<"(main):received option --oinfo='"<<optarg<<"'"<<endl;
					uniset_init(argc,argv,conffile);
					UniversalInterface ui(conf);
					return oinfo(optarg,ui);
				}
				break;

				case 'e':	//--exist
				{
//					cout<<"(main):received option --exist"<<endl;
					uniset_init(argc,argv,conffile);
					UniversalInterface ui(conf);
				
					Command cmd=Exist;
					ObjectRepository* rep = new ObjectRepository(conf);
					commandToAll(conf->getServicesSection(), rep, (Command)cmd);
					commandToAll(conf->getControllersSection(), rep, (Command)cmd);
					commandToAll(conf->getObjectsSection(), rep, (Command)cmd);
					delete rep;
//					cout<<"(exist): done"<<endl;
				}
				return 0;

				case 's':	//--start
				{
//					cout<<"(main):received option --start"<<endl;
					uniset_init(argc,argv,conffile);
					UniversalInterface ui(conf);

					Command cmd=StartUp;
					ObjectRepository* rep = new ObjectRepository(conf);
					commandToAll(conf->getServicesSection(), rep, (Command)cmd);
					commandToAll(conf->getControllersSection(), rep, (Command)cmd);
					commandToAll(conf->getObjectsSection(), rep, (Command)cmd);
					delete rep;
				}
				return 0;

				case 'r':	//--configure
				{
					uniset_init(argc,argv,conffile);
					UniversalInterface ui(conf);
					return configure(optarg,ui);
				}
				break;

				case 'f':	//--finish
				{
//					cout<<"(main):received option --finish"<<endl;
					uniset_init(argc,argv,conffile);
					UniversalInterface ui(conf);

					Command cmd=Finish;
					ObjectRepository* rep = new ObjectRepository(conf);
					commandToAll(conf->getServicesSection(), rep, (Command)cmd);
					commandToAll(conf->getControllersSection(), rep, (Command)cmd);
					commandToAll(conf->getObjectsSection(), rep, (Command)cmd);
					delete rep;
					cout<<"(finish): done"<<endl;
				}
				return 0;

				case 'l':	//--logrotate
				{
					uniset_init(argc,argv,conffile);
					UniversalInterface ui(conf);
					return logRotate(optarg, ui);
				}
				break;

				case 'y':	//--getCalibrate
				{
//					cout<<"(main):received option --getCalibrate='"<<optarg<<"'"<<endl;
					uniset_init(argc,argv,conffile);
					UniversalInterface ui(conf);
					return getCalibrate(optarg, ui);
				}
				break;

				case 'u':	//--foldUp
				{
//					cout<<"(main):received option --foldUp"<<endl;
					uniset_init(argc,argv,conffile);
					UniversalInterface ui(conf);
				
					Command cmd=FoldUp;
					ObjectRepository* rep = new ObjectRepository(conf);
					commandToAll(conf->getServicesSection(), rep, (Command)cmd);
					commandToAll(conf->getControllersSection(), rep, (Command)cmd);
					commandToAll(conf->getObjectsSection(), rep, (Command)cmd);
					delete rep;
//					cout<<"(foldUp): done"<<endl;
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
	catch(Exception& ex)
	{
		cout <<"admin(main): " << ex << endl;
	}
	catch(CORBA::SystemException& ex)
    {
		cerr << "поймали CORBA::SystemException:" << ex.NP_minorString() << endl;
    }
    catch(CORBA::Exception&)
    {
		cerr << "поймали CORBA::Exception." << endl;
    }
    catch(omniORB::fatalException& fe)
    {
		cerr << "поймали omniORB::fatalException:" << endl;
        cerr << "  file: " << fe.file() << endl;
		cerr << "  line: " << fe.line() << endl;
        cerr << "  mesg: " << fe.errmsg() << endl;
    }
    catch(...)
    {
		cerr << "неизвестное исключение" << endl;
    }

    return 1;
}

// ==============================================================================================
static bool commandToAll(const string section, ObjectRepository *rep, Command cmd)
{
	cout <<"\n||=======********  " << section << "  ********=========||\n"<< endl;
	
	try
	{
		ListObjectName ls;
	  	rep->list(section.c_str(),&ls);
		if(ls.empty())
		{
			cout << "пусто!!!!!!" << endl;
			return false;
		}

		ObjectsManager_i_var proc;		
		UniSetObject_i_var obj;
		string fullName;
		ListObjectName::const_iterator li;
		string buf;
		
		cout.setf(ios::left, ios::adjustfield);
		for ( li=ls.begin();li!=ls.end();++li)
		{
			string ob(*li);
			buf = section+"/"+ob;
			fullName= buf.c_str();
			try
			{
				UniSetTypes::ObjectVar o =rep->resolve(fullName);
				obj= UniSetObject_i::_narrow(o);
								
				switch( cmd )
				{
					case StartUp:
					{
						if(CORBA::is_nil(obj))	break;
						SystemMessage msg(SystemMessage::StartUp);
						obj->push( Message::transport(msg) );
						cout << setw(55) << ob <<"   <--- start OK" <<   endl;
					}
					break;
					
					case FoldUp:
					{
						if(CORBA::is_nil(obj))	break;
						SystemMessage msg(SystemMessage::FoldUp);
						obj->push( Message::transport(msg) );
						cout << setw(55) << ob << "   <--- foldUp OK" <<   endl;
					}
					break;

					case Finish:
					{
						if(CORBA::is_nil(obj))	break;
						SystemMessage msg(SystemMessage::Finish);
						obj->push( Message::transport(msg) );							
						cout << setw(55)<< ob << "   <--- finish OK" <<   endl;
					}
					break;
						
					case Exist:
					{
						if (obj->exist())
							cout << setw(55) << ob << "   <--- exist ok\n";
						else
							cout << setw(55) << ob << "   <--- exist NOT OK\n";
					}
					break;

					case Configure:
					{
						SystemMessage sm(SystemMessage::ReConfiguration); 
						obj->push(sm.transport_msg());
						cout << setw(55) << ob << "   <--- configure ok\n";
					}
					break;

					case LogRotate:
					{
						SystemMessage msg(SystemMessage::LogRotate);
						obj->push( Message::transport(msg) );
						cout << setw(55) << ob << "   <--- logrotate ok\n";
						break;
					}
						
					default:
					{
						cout << "неизвестная команда -" << cmd << endl;
						return false;
					}	
				}
			}
			catch(Exception& ex)
			{
				cout << setw(55) << ob << "   <--- " << ex << endl;
			}
			catch( CORBA::SystemException& ex )
			{
				cout << setw(55) << ob  << "   <--- недоступен!!(CORBA::SystemException): " << ex.NP_minorString() << endl;
			}
		}
	}
	catch( ORepFailed )
	{
  		return false;
	}

	return true;
}

// ==============================================================================================
static void createSections( UniSetTypes::Configuration* rconf )
{
	ObjectRepositoryFactory repf(rconf);

	repf.createRootSection(rconf->getRootSection());
	repf.createRootSection(rconf->getSensorsSection());
	repf.createRootSection(rconf->getObjectsSection());
	repf.createRootSection(rconf->getControllersSection());
	repf.createRootSection(rconf->getServicesSection());
	cout<<"(create): created"<<endl;	
}

// ==============================================================================================
int omap()
{
	try
	{
		cout.setf(ios::left, ios::adjustfield);
		cout << "========================== ObjectsMap  =================================\n";	
		conf->oind->printMap(cout);
		cout << "==========================================================================\n";	
	}
	catch(Exception& ex)
	{
		unideb[Debug::CRIT] << " configuration init  FAILED!!! \n";
		return 1;
	}
	return 0;
}

// --------------------------------------------------------------------------------------
int msgmap()
{
	try
	{
		cout.setf(ios::left, ios::adjustfield);
		cout << "========================== MessagesMap  =================================\n";	
		conf->mi->printMessagesMap(cout);
		cout << "==========================================================================\n";	
	}
	catch(Exception& ex)
	{
		unideb[Debug::CRIT] << " configuration init  FAILED!!! " << ex << endl;;
		return 1;
	}
	
	return 0;
}

// --------------------------------------------------------------------------------------
int setValue( const string args, UniversalInterface &ui, Configuration* conf )
{
	int err = 0;
	
	typedef std::list<UniSetTypes::ParamSInfo> SList;
	SList sl = UniSetTypes::getSInfoList(args, conf);
	cout << "====== setValue ======" << endl;
	for( SList::iterator it=sl.begin(); it!=sl.end(); it++ )
	{	
		try
		{
			UniversalIO::IOTypes t = conf->getIOType(it->si.id);
			cout << "  value: " << it->val << endl;
			cout << "   name: (" << it->si.id << ") " << it->fname << endl;
			cout << "   iotype: " << t << endl;
			cout << "   text: " << conf->oind->getTextName(it->si.id) << "\n\n";
			
			if( it->si.node == DefaultObjectId )
				it->si.node = conf->getLocalNode();
			
			switch(t)
			{
				case UniversalIO::DigitalInput:
					ui.saveState(it->si.id,(it->val?true:false),t,it->si.node);
				break;
				
				case UniversalIO::DigitalOutput:
					ui.setState(it->si.id,(it->val?true:false),it->si.node);
				break;
				
				case UniversalIO::AnalogInput:
					ui.saveValue(it->si.id,it->val,t,it->si.node);
				break;
				
				case UniversalIO::AnalogOutput:
					ui.setValue(it->si.id,it->val,it->si.node);
				break;
				
				default:
					cerr << "FAILED: Unknown 'iotype' for " << it->fname << endl;
					err = 1;
					break;
			}
		}
		catch(Exception& ex)
		{
			cerr << "(setValue): " << ex << endl;;
			err = 1;
		}	
	}
	
	return err;
}

// --------------------------------------------------------------------------------------
int getValue( const string args, UniversalInterface &ui, Configuration* conf )
{
	int err = 0;
	
	typedef std::list<UniSetTypes::ParamSInfo> SList;
	SList sl = UniSetTypes::getSInfoList( args, UniSetTypes::conf );
	cout << "====== getValue ======" << endl;
	for( SList::iterator it=sl.begin(); it!=sl.end(); it++ )
	{	
		try
		{
			UniversalIO::IOTypes t = conf->getIOType(it->si.id);
			cout << "   name: (" << it->si.id << ") " << it->fname << endl;
			cout << "   iotype: " << t << endl;
			cout << "   text: " << conf->oind->getTextName(it->si.id) << "\n\n";
			
			if( it->si.node == DefaultObjectId )
				it->si.node = conf->getLocalNode();
			
			switch(t)
			{
				case UniversalIO::DigitalOutput:
				case UniversalIO::DigitalInput:
					cout << "  state: " << ui.getState(it->si.id,it->si.node) << endl;
				break;
				
				case UniversalIO::AnalogOutput:
				case UniversalIO::AnalogInput:
					cout << "  value: " << ui.getValue(it->si.id,it->si.node) << endl;
				break;
				
				default:
					cerr << "FAILED: Unknown 'iotype' for " << it->fname << endl;
					err = 1;
					break;
			}
		}
		catch(Exception& ex)
		{
			cerr << "(getValue): " << ex << endl;
			err = 1;
		}	
	}
	
	return err;
}
// --------------------------------------------------------------------------------------
int getCalibrate( const std::string args, UniversalInterface &ui )
{
	int err = 0;
  	typedef std::list<UniSetTypes::ParamSInfo> SList;
	SList sl = UniSetTypes::getSInfoList( args, UniSetTypes::conf );
	cout << "====== getCalibrate ======" << endl;
	for( SList::iterator it=sl.begin(); it!=sl.end(); it++ )
	{	
		if( it->si.node == DefaultObjectId )
			it->si.node = conf->getLocalNode();
	  
		cout << "      name: (" << it->si.id << ") " << it->fname << endl;
		cout << "      text: " << conf->oind->getTextName(it->si.id) << "\n";
		try
		{
			cout << "калибровка: ";
			IOController_i::CalibrateInfo ci = ui.getCalibrateInfo(it->si);
			cout << ci << endl;
		}
		catch(Exception& ex)
		{
			cerr << "(getCalibrate): " << ex << endl;;
			err = 1;
		}	
	}
	
	return err;
}

// --------------------------------------------------------------------------------------
int getRawValue( const std::string args, UniversalInterface &ui )
{
	int err = 0;
  	typedef std::list<UniSetTypes::ParamSInfo> SList;
	SList sl = UniSetTypes::getSInfoList( args, UniSetTypes::conf );
	cout << "====== getRawValue ======" << endl;
	for( SList::iterator it=sl.begin(); it!=sl.end(); it++ )
	{	
		if( it->si.node == DefaultObjectId )
			it->si.node = conf->getLocalNode();	  
		
		cout << "   name: (" << it->si.id << ") " << it->fname << endl;
		cout << "   text: " << conf->oind->getTextName(it->si.id) << "\n\n";
		try
		{
			cout << "  value: " << ui.getRawValue(it->si) << endl;
		}
		catch(Exception& ex)
		{
			cerr << "(getRawValue): " << ex << endl;;
			err = 1;
		}	
	}
	return err;
}

// --------------------------------------------------------------------------------------
int logRotate( const string arg, UniversalInterface &ui )
{
	// посылка всем
	if( arg.empty() || (arg.c_str())[0]!='-' )
	{
		ObjectRepository* rep = new ObjectRepository(conf);
		commandToAll(conf->getServicesSection(), rep, (Command)LogRotate);
		commandToAll(conf->getControllersSection(), rep, (Command)LogRotate);
		commandToAll(conf->getObjectsSection(), rep, (Command)LogRotate);
	 	delete rep;
	}
	else // посылка определённому объекту
	{
		UniSetTypes::ObjectId id = conf->oind->getIdByName(arg);
		if( id == DefaultObjectId )
		{
			cout << "(logrotate): name='" << arg << "' не найдено!!!\n";
			return 1;
		}
			
		SystemMessage sm(SystemMessage::LogRotate);
		TransportMessage tm(sm.transport_msg());
		ui.send(id,tm);
		cout << "\nSend 'LogRotate' to " << arg << " OK.\n";
	}
	return 0;
}

// --------------------------------------------------------------------------------------
int configure( const string arg, UniversalInterface &ui )
{
	// посылка всем
	if( arg.empty() || (arg.c_str())[0]!='-' )
	{
		ObjectRepository* rep = new ObjectRepository(conf);
		commandToAll(conf->getServicesSection(), rep, (Command)Configure);
		commandToAll(conf->getControllersSection(), rep, (Command)Configure);
		commandToAll(conf->getObjectsSection(), rep, (Command)Configure);
	 	delete rep;
	}
	else // посылка определённому объекту
	{
		UniSetTypes::ObjectId id = conf->oind->getIdByName(arg);
		if( id == DefaultObjectId )
		{
			cout << "(configure): name='" << arg << "' не найдено!!!\n";
			return 1;
		}
		SystemMessage sm(SystemMessage::ReConfiguration);
		TransportMessage tm(sm.transport_msg());
		ui.send(id,tm);
		cout << "\nSend 'ReConfigure' to " << arg << " OK.\n";
	}
	return 0;			
}

// --------------------------------------------------------------------------------------
int oinfo( const string arg, UniversalInterface &ui )
{
	UniSetTypes::ObjectId oid(uni_atoi(arg));
	if( oid==0 )
	{
		cout << "(oinfo): Не задан OID!"<< endl;
		return 1;
	}
			
	UniSetTypes::ObjectVar o = ui.resolve(oid);
	UniSetObject_i_var obj = UniSetObject_i::_narrow(o);
	if(CORBA::is_nil(obj))
	{
		cout << "(oinfo): объект " << oid << " недоступен" << endl;
	}
	else
	{
		SimpleInfo_var inf = obj->getInfo();
		cout << inf->info << endl;
	}
			
	return 0;
}

// --------------------------------------------------------------------------------------
