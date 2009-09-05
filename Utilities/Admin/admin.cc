// $Id: admin.cc,v 1.27 2008/11/29 21:24:24 vpashka Exp $

// --------------------------------------------------------------------------
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <getopt.h>
// --------------------------------------------------------------------------
#include "ORepHelpers.h"
#include "ObjectRepository.h"
#include "ObjectRepositoryFactory.h"
#include "Exceptions.h"
#include "UniSetObject.h"
#include "ObjectsManager.h"
#include "MessageType.h"
//#include "DBServer.h"
//#include "InfoServer.h"
#include "Configuration.h"
#include "ObjectIndex_XML.h"
#include "Debug.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// --------------------------------------------------------------------------
// \todo п╫п╟п╢п╬ п©п╣я─п╣п©п╦я│п╟я┌я▄ я─п╣п╟п╩п╦п╥п╟я├п╦я▌ !!!!
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
	{ "alarm", required_argument, 0, 'a' },
	{ "anotify", required_argument, 0, 'n' },
	{ "dnotify", required_argument, 0, 'd' },
	{ "info", required_argument, 0, 'i' },
	{ "saveValue", required_argument, 0, 'v' },
	{ "saveState", required_argument, 0, 't' },
	{ "setValue", required_argument, 0, 'x' },
	{ "setState", required_argument, 0, 'j' },
	{ "getValue", required_argument, 0, 'g' },
	{ "getState", required_argument, 0, 'k' },
	{ "getRawValue", required_argument, 0, 'w' },
	{ "getCalibrate", required_argument, 0, 'y' },
	{ "oinfo", required_argument, 0, 'p' },
	{ NULL, 0, 0, 0 }
};

string conffile("configure.xml");

// --------------------------------------------------------------------------
static bool commandToAll(const string section, ObjectRepository *rep, Command cmd);
static void createSections(UniSetTypes::Configuration* c);
static bool separateArgs(string &args, string &arg);
//static bool getID( const string arg, ObjectId id, ObjectId node );
// --------------------------------------------------------------------------
int omap();
int msgmap();
int configure( string args, UniversalInterface &ui );
int logRotate( string args, UniversalInterface &ui );
int alarm( string args, UniversalInterface &ui );
int anotify( string args, UniversalInterface &ui );
int dnotify( string args, UniversalInterface &ui );
int info( string args, UniversalInterface &ui );
int saveValue( string args, UniversalInterface &ui );
int saveState( string args, UniversalInterface &ui );
int setValue( string args, UniversalInterface &ui );
int setState( string args, UniversalInterface &ui );
int getValue( string args, UniversalInterface &ui );
int getRawValue( string args, UniversalInterface &ui );
int getState( string args, UniversalInterface &ui );
int getCalibrate( string args, UniversalInterface &ui );
int oinfo( string args, UniversalInterface &ui );

// --------------------------------------------------------------------------
static void print_help(int width, const string cmd, const string help, const string tab=" " )
{
	// чтобы не менчять параметры основного потока
	// создаём свой stream...
	ostringstream info;
	info.setf(ios::left, ios::adjustfield);
	info << tab << setw(width) << cmd << " - " << help;
	cout << info.str();
}
// --------------------------------------------------------------------------
static void short_usage()
{
	cout << "Usage: uniset-admin [--confile configure.xml] --command [arg]	\n for detailed information arg --help";
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
	print_help(36, "-a|--alarm [code,cause,character] ","Посылка AlarmMessage(character: 1-Normal, 2-Warning, 3-Alarm, default: Alarm)\n");
	print_help(36, "-i|--info [code] ","Посылка InfoMessage\n");
	print_help(36, "-p|--oinfo OID ","Получить информацию об объекте (SimpleInfo).\n");	
	cout << endl;
	print_help(48, "-n|--anotify ObjectId,SensorId,value ","Посылка SensorMessage (аналоговый датчик) объекту ObjectId\n");
	print_help(48, "-d|--dnotify ObjectId,SensorId,state ","Посылка SensorMessage (дискретный датчик) объекту ObjectId\n");	
	print_help(48, "-v|--saveValue SensorId=value[,SId2=v2,...] ","Установить значение аналогоых датчиков SensorId в значение value.\n");
	print_help(48, "-t|--saveState SensorId=state[,SId2=s2,...] ","Установить значение дискретных датчиков SensorId в значение state.\n");	
	print_help(48, "-x|--setValue OutputId=value[,SId2=v2,...] ","Подать на аналоговые выходы OuputId значение value.\n");
	print_help(48, "-j|--setState OutputId=state[,SId2=s2,...] ","Подать на дискретные выходы OuputId значение state.\n");	
	cout << endl;
	print_help(36, "-g|--getValue SensorId[,SId2,...] ","Получить значение аналоговых датчиков SensorId.\n");
	print_help(36, "-k|--getState SensorId[,SId2,...] ","Получить значение дискретных датчиков SensorId.\n");
	print_help(36, "-w|--getRawValue SensorId? ","Получить значение аналогового датчика RawSensorId?.\n");
	print_help(36, "-y|--getCalibrate SensorId? ","Получить калибровачную диаграмму?.\n");
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


	while( (opt = getopt_long(argc, argv, "hc:beomsfur:l:a:n:d:i:v:t:x:j:g:k:w:p:y:",longopts,&optindex)) != -1 ) 
	{
		switch (opt) //разбираем параметры 
		{
			case 'h':	//--help
			{
//				cout<<"(main):received option --help"<<endl;
				usage();
			}
			return 0;

			case 'c':	//--confile
			{
//				cout<<"(main):received option --confile='"<<optarg<<"'"<<endl;
				conffile = optarg;
			}
			break;
			
			case 'o':	//--omap
			{
//				cout<<"(main):received option --omap"<<endl;
				uniset_init(argc,argv,conffile);
				return omap();
			}
			break;			

			case 'b':	//--create
			{
//				cout<<"(main):received option --create"<<endl;
				uniset_init(argc,argv,conffile);
				createSections(conf);
			}
			return 0;
			
			case 'm':	//--msgmap
			{
//				cout<<"(main):received option --msgmap"<<endl;
				uniset_init(argc,argv,conffile);
				return msgmap();
			}	
			break;

			case 'a':	//--alarm
			{
//				cout<<"(main):received option --alarm='"<<optarg<<"'"<<endl;
				uniset_init(argc,argv,conffile);
				UniversalInterface ui(conf);
				return alarm(optarg,ui);
			}
			break;

			case 'i':	//--info
			{
//				cout<<"(main):received option -info='"<<optarg<<"'"<<endl;
				uniset_init(argc,argv,conffile);
				UniversalInterface ui(conf);
				return info(optarg,ui);
			}
			break;

			case 'n':	//--anotify
			{
//				cout<<"(main):received option --anotify='"<<optarg<<"'"<<endl;
				uniset_init(argc,argv,conffile);
				UniversalInterface ui(conf);
				return anotify(optarg,ui);
			}
			break;

			case 'd':	//--dnotify
			{
//				cout<<"(main):received option --dnotify='"<<optarg<<"'"<<endl;
				uniset_init(argc,argv,conffile);
				UniversalInterface ui(conf);
				return dnotify(optarg,ui);
			}
			break;

			case 'v':	//--saveValue
			{
//				cout<<"(main):received option --saveValue='"<<optarg<<"'"<<endl;
				uniset_init(argc,argv,conffile);
				UniversalInterface ui(conf);
				return saveValue(optarg,ui);
			}
			break;

			case 't':	//--saveState
			{
//				cout<<"(main):received option --saveState='"<<optarg<<"'"<<endl;
				uniset_init(argc,argv,conffile);
				UniversalInterface ui(conf);
				return saveState(optarg,ui);
			}
			break;

			case 'x':	//--setValue
			{
//				cout<<"(main):received option --setValue='"<<optarg<<"'"<<endl;
				uniset_init(argc,argv,conffile);
				UniversalInterface ui(conf);
				return setValue(optarg,ui);
			}
			break;

			case 'j':	//--setState
			{
//				cout<<"(main):received option --setState='"<<optarg<<"'"<<endl;
				uniset_init(argc,argv,conffile);
				UniversalInterface ui(conf);
				return setState(optarg,ui);
			}
			break;

			case 'g':	//--getValue
			{
//				cout<<"(main):received option --getValue='"<<optarg<<"'"<<endl;
				uniset_init(argc,argv,conffile);
				UniversalInterface ui(conf);
				return getValue(optarg,ui);
			}
			break;

			case 'k':	//--getState
			{
//				cout<<"(main):received option --getState='"<<optarg<<"'"<<endl;
				uniset_init(argc,argv,conffile);
				UniversalInterface ui(conf);
				return getState(optarg,ui);
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
//				cout<<"(main):received option --oinfo='"<<optarg<<"'"<<endl;
				uniset_init(argc,argv,conffile);
				UniversalInterface ui(conf);
				return oinfo(optarg,ui);
			}
			break;

			case 'e':	//--exist
			{
//				cout<<"(main):received option --exist"<<endl;
				uniset_init(argc,argv,conffile);
				UniversalInterface ui(conf);
				
				Command cmd=Exist;	
				ObjectRepository* rep = new ObjectRepository(conf);
				commandToAll(conf->getServicesSection(), rep, (Command)cmd);
				commandToAll(conf->getControllersSection(), rep, (Command)cmd);
				commandToAll(conf->getObjectsSection(), rep, (Command)cmd);
			 	delete rep;
//				cout<<"(exist): done"<<endl;
			}
			return 0;

			case 's':	//--start
			{
//				cout<<"(main):received option --start"<<endl;
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
//				cout<<"(main):received option --configure='"<<optarg<<"'"<<endl;
				uniset_init(argc,argv,conffile);
				UniversalInterface ui(conf);
				
				return configure(optarg,ui);
/*				
				Command cmd=Configure;	
				ObjectRepository* rep = new ObjectRepository(conf);
				commandToAll(conf->getServicesSection(), rep, (Command)cmd);
				commandToAll(conf->getControllersSection(), rep, (Command)cmd);
				commandToAll(conf->getObjectsSection(), rep, (Command)cmd);
			 	delete rep;
*/			}
			break;

			case 'f':	//--finish
			{
//				cout<<"(main):received option --finish"<<endl;
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
//				cout<<"(main):received option --logrotate='"<<optarg<<"'"<<endl;
				uniset_init(argc,argv,conffile);
				UniversalInterface ui(conf);
				return logRotate(optarg, ui);
/*				
				Command cmd=LogRotate;	
				ObjectRepository* rep = new ObjectRepository(conf);
				commandToAll(conf->getServicesSection(), rep, (Command)cmd);
				commandToAll(conf->getControllersSection(), rep, (Command)cmd);
				commandToAll(conf->getObjectsSection(), rep, (Command)cmd);
			 	delete rep;
*/			}
			break;

			case 'y':	//--getCalibrate
			{
//				cout<<"(main):received option --getCalibrate='"<<optarg<<"'"<<endl;
				uniset_init(argc,argv,conffile);
				UniversalInterface ui(conf);
				return getCalibrate(optarg, ui);
			}
			break;

			case 'u':	//--foldUp
			{
//				cout<<"(main):received option --foldUp"<<endl;
				uniset_init(argc,argv,conffile);
				UniversalInterface ui(conf);
				
				Command cmd=FoldUp;	
				ObjectRepository* rep = new ObjectRepository(conf);
				commandToAll(conf->getServicesSection(), rep, (Command)cmd);
				commandToAll(conf->getControllersSection(), rep, (Command)cmd);
				commandToAll(conf->getObjectsSection(), rep, (Command)cmd);
			 	delete rep;
//				cout<<"(foldUp): done"<<endl;
			}
			return 0;

			case '?':
			default:
			{
				short_usage();
				return 0;
			}
				
		}	
	}

		cout <<"\nвсе."<< endl;
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
bool separateArgs(string &args,string &arg)
{
	int ind;

	if(args=="")
		return false;
	
	ind = args.find_first_of(",");
	arg = args.substr(0,ind);
	args = args.substr(ind+1,args.size());

	if (args==arg)
		args="";

	return true;
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
int alarm(string args, UniversalInterface &ui)
{
	string arg="";

	if( args.size() == 0 || strncmp(args.c_str(),"-",1)==0 )
	{
		AlarmMessage am(UniSetTypes::DefaultObjectId, "Administrator Alarm: Тестовое сообщение",
						UniSetTypes::DefaultObjectId, conf->getLocalNode());

		TransportMessage tm(am.transport_msg());
		cout <<  "Administrator Alarm: Тестовое сообщение " << endl;
		ui.send(conf->getInfoServer(), tm);
		return 0;
	}
	
	if(separateArgs(args,arg))
	{
		UniSetTypes::MessageCode code = uni_atoi(arg);
		UniSetTypes::MessageCode cause(UniSetTypes::DefaultMessageCode);

		if(separateArgs(args,arg))
		{
			cause = uni_atoi(arg);
		}
		else
		{
			cerr<<"(alarm): cause is not specified ! \n";
			unideb[Debug::WARN] << "cause is not specified ! \n";
			return 1;
		}
	
		AlarmMessage::Character ch(AlarmMessage::Alarm);
		
		int chid ;
//		cout<<"!!!!!! args="<<args<<" arg="<<arg<<" cause="<<cause<<" code="<<code<<endl;
			
		if( sscanf( args.c_str(),"%d",&chid) < 1 )
		{
			cerr<<"(alarm): ch is not specified ! \n";
			unideb[Debug::WARN] << "ch is not specified ! \n";
			return 1;
		}

		switch(chid)
		{
			case 1:
				ch = AlarmMessage::Normal;
			break;
					
			case 2:
				ch = AlarmMessage::Warning;
			break;

			case 3:
				ch = AlarmMessage::Alarm;
			break;
		}
		cout << "alarm character(" << chid << "):\t" << ch << endl;
				
		AlarmMessage am(UniSetTypes::DefaultObjectId,code,cause, conf->getLocalNode(), ch);
		TransportMessage tm(am.transport_msg());

		cout << "alarm message(" << code << "):\t";
		cout << conf->mi->getMessage(code) << endl;
		if( cause )
		{
			cout << "alarm cause(" << cause << "):\t";
			cout << conf->mi->getMessage(cause) << endl;
		}
		cout << endl;
		ui.send(conf->getInfoServer(), tm);
		cout << "send alarm OK" <<   endl;
	}
	else
	{
		cerr<<"(alarm): params are not specified ! \n";
		unideb[Debug::WARN] << "params are not specified ! \n";
		return 1;		
	}
	
	return 0;
}

// --------------------------------------------------------------------------------------
int info(string arg, UniversalInterface &ui)
{
	if( arg.size() == 0 || strncmp(arg.c_str(),"-",1)==0 )
	{
		InfoMessage im(UniSetTypes::DefaultObjectId, "Administrator Info: Тестовое сообщение");
		TransportMessage tm(im.transport_msg());
		cout <<  "Administrator Info: Тестовое сообщение " << endl;
		ui.send(conf->getInfoServer(), tm);
		return 0;
	}
			
	int icode;
	if( sscanf( arg.c_str(),"%d",&icode) < 1 )
	{
		cerr<<"(info): code is not specified ! \n";
		unideb[Debug::WARN] << "code is not specified ! \n";
		return 1;		
	}
	
	UniSetTypes::MessageCode code = icode;
	InfoMessage im(UniSetTypes::DefaultObjectId, code);
	TransportMessage tm(im.transport_msg());
	cout << "info: (" << code << ") ";
	cout << conf->mi->getMessage(code);
	ui.send(conf->getInfoServer(), tm);
	cout << "send info OK" <<   endl;

	return 0;
}

// --------------------------------------------------------------------------------------
int anotify(string args, UniversalInterface &ui)
{
	UniSetTypes::ObjectId id;
	UniSetTypes::ObjectId sid;
	long val;

	if( sscanf( args.c_str(),"%ld,%ld,%ld",&id,&sid,&val ) < 3 )
	{
		cerr << "(anotify): Неверный параметр ObjectId,SensorId,Value"<< endl;
		return 1;
	}
		
	cout << "anotify --------\n";	
	cout << "  value: " << val << endl;
	cout << " sensor: (" << sid << ") " << conf->oind->getMapName(sid) << endl;
	cout << "   text: " << conf->oind->getTextName(sid) << endl;
	cout << "адресат: " << conf->oind->getMapName(id) << "\n"<<endl;

	SensorMessage sm(sid,(long)val);
	sm.consumer = id;
	TransportMessage tm(sm.transport_msg());
	ui.send(id,tm);

	return 0;
}
// --------------------------------------------------------------------------------------
int dnotify(string args, UniversalInterface &ui)
{
	UniSetTypes::ObjectId id;
	UniSetTypes::ObjectId sid;
	int ival;
	bool val;

	if( sscanf( args.c_str(),"%ld,%ld,%d",&id,&sid,&ival ) < 3 )
	{
		cerr << "(anotify): Неверный параметр ObjectId,SensorId,State"<< endl;
		return 1;
	}
	if (ival == 0)
		val = false;
	else if( ival == 1 )
		val = true;
	else	
	{
		cerr << "(anotify): Неверный параметр State. Должен быть булевым!"<< endl;
		return 1;
	}

	cout << "dnotify --------\n";
	cout << "  state: " << val << endl;
	cout << " sensor: (" << sid << ") " << conf->oind->getMapName(sid) << endl;
	cout << "   text: " << conf->oind->getTextName(sid) << endl;
	cout << "адресат: " << conf->oind->getMapName(id) << "\n\n";

	SensorMessage sm(sid,(bool)val);
	sm.consumer = id;
	TransportMessage tm(sm.transport_msg());
	ui.send(id,tm);

	return 0;
}

// --------------------------------------------------------------------------------------
int saveValue(string args, UniversalInterface &ui)
{
	int err;
	err=0;
	string arg;
	UniSetTypes::ObjectId sid = DefaultObjectId;
	UniSetTypes::ObjectId node = DefaultObjectId;
	long val;

	cout << "====== saveValue ======" << endl;
	for(int i=1;separateArgs(args,arg);i++)
	{
		if( isdigit( arg[0] ) )
		{
			if( sscanf( arg.c_str(),"%ld=%ld",&sid,&val ) < 2 )
			{
				cout << i <<"\t------------------------"<< endl;
				cerr << "(digit): !!! пара SensorId=Value #"<<i<<" '"<<arg<<"' задана неверно!!!!!!\n"<< endl;
				err=1;
				continue;
			}
		}
		else
		{
			int ind;
			string strval;
			ind = arg.find_first_of("=");
			string name(arg.substr(0,ind));
			strval = arg.substr( ind + 1, arg.length() );
			sid = conf->getSensorID(name);

			if( sid == UniSetTypes::DefaultObjectId || (sscanf( strval.c_str(),"%ld",&val ) < 1) )
			{
				cout << i <<"\t------------------------"<< endl;
				cerr << "(name): !!! пара SensorName=Value #"<<i<<" '"<<arg<<"' задана неверно!!!!!!\n"<< endl;
				err=1;
				continue;
			}
		}
		cout << i <<"\t------------------------"<< endl;
		try
		{		
//			cout <<"\n\t"<<sid<<"\t"<<val<<endl;
			cout << "  value: " << val << endl;
			cout << "   name: (" << sid << ") " << conf->oind->getMapName(sid) << endl;
			cout << "   text: " << conf->oind->getTextName(sid) << "\n\n";
			if( node == DefaultObjectId )
				node = conf->getLocalNode();

			ui.saveValue(sid,val,UniversalIO::AnalogInput,node);
		}
		catch(Exception& ex)
		{
			unideb[Debug::CRIT] << "(saveValue): " << ex << endl;;
			err = 1;
		}	
	}
	return err;
}

// --------------------------------------------------------------------------------------
int saveState(string args, UniversalInterface &ui)
{
	int err;
	err=0;
	string arg;
	UniSetTypes::ObjectId sid(DefaultObjectId);
	long inval;
	bool val;

	cout << "====== saveState ======" << endl;
	for(int i=1;separateArgs(args,arg);i++)
	{
		if( isdigit( arg[0] ) )
		{
			if( sscanf( arg.c_str(),"%ld=%ld",&sid,&inval ) < 2 )
			{
				cout << i <<"\t------------------------"<< endl;		
				cerr << "!!!!!!!!! пара SensorId=State #"<<i<<" '"<<arg<<"' задана неверно!!!!!!\n"<< endl;
				err=1;
				continue;
			}
		}
		else
		{
			int ind;
			string strval;
			ind = arg.find_first_of("=");
			string name(arg.substr(0,ind));
			strval = arg.substr( ind + 1, arg.length() );
			sid = conf->getSensorID(name);

			if( sid == UniSetTypes::DefaultObjectId || (sscanf( strval.c_str(),"%ld",&inval ) < 1) )
			{
				cout << i <<"\t------------------------"<< endl;		
				cerr << "!!!!!!!!! пара SensorName=State #"<<i<<" '"<<arg<<"' задана неверно!!!!!!\n"<< endl;
				err=1;
				continue;
			}
		}
		if(inval==1)
			val=true;
		else if(inval==0)
			val=false;
		else
		{
			cout << i <<"\t------------------------"<< endl;
			cerr << "!!!!!!!!! State в паре SensorId=State #"<<i<<" '"<<arg<<"' должен быть булевым !!!!!!\n"<< endl;
			err=1;
			continue;
		}		

		cout << i <<"\t------------------------"<< endl;		
		try
		{		
//			cout <<"\n\t"<<sid<<"\t"<<val<<endl;		
			cout << "  state: " << val << endl;
			cout << "   name: (" << sid << ") " << conf->oind->getMapName(sid) << endl;
			cout << "   text: " << conf->oind->getTextName(sid) << "\n\n";
			ui.saveState(sid,val,UniversalIO::DigitalInput);
		}
		catch(Exception& ex)
		{
			unideb[Debug::CRIT] << "(saveState): " << ex << endl;;
			err = 1;
		}	
	}
	return err;
}

// --------------------------------------------------------------------------------------
int setValue(string args, UniversalInterface &ui)
{
	int err;
	err=0;
	string arg;
	UniSetTypes::ObjectId sid(DefaultObjectId);
	long val;

	cout << "====== setValue ======" << endl;
	for(int i=1;separateArgs(args,arg);i++)
	{
		if( isdigit( arg[0] ) )
		{
			if( sscanf( arg.c_str(),"%ld=%ld",&sid,&val ) < 2 )
			{
				cout << i <<"\t------------------------"<< endl;		
				cerr << "!!!!!!!!! пара SensorId=Value #"<<i<<" '"<<arg<<"' задана неверно!!!!!!\n"<< endl;
				err=1;
				continue;
			}
		}
		else
		{
			int ind;
			string strval;
			ind = arg.find_first_of("=");
			string name(arg.substr(0,ind));
			strval = arg.substr( ind + 1, arg.length() );
			sid = conf->getSensorID(name);

			if( sid == UniSetTypes::DefaultObjectId || (sscanf( strval.c_str(),"%ld",&val ) < 1) )
			{
				cout << i <<"\t------------------------"<< endl;		
				cerr << "!!!!!!!!! пара SensorName=Value #"<<i<<" '"<<arg<<"' задана неверно!!!!!!\n"<< endl;
				err=1;
				continue;
			}
		}


		cout << i <<"\t------------------------"<< endl;		
		try
		{		
//			cout <<"\n\t"<<sid<<"\t"<<val<<endl;		
			cout << "  value: " << val << endl;
			cout << "   name: (" << sid << ") " << conf->oind->getMapName(sid) << endl;
			cout << "   text: " << conf->oind->getTextName(sid) << "\n\n";
			ui.setValue(sid,val);
		}
		catch(Exception& ex)
		{
			unideb[Debug::CRIT] << "(setValue): " << ex << endl;;
			err = 1;
		}	
	}
	return err;
}

// --------------------------------------------------------------------------------------
int setState(string args, UniversalInterface &ui)
{
	int err;
	err=0;
	string arg;
	UniSetTypes::ObjectId sid(DefaultObjectId);
	long inval;
	bool val;

	cout << "====== setState ======" << endl;
	for(int i=1;separateArgs(args,arg);i++)
	{
		if( isdigit( arg[0] ) )
		{
			if( sscanf( arg.c_str(),"%ld=%ld",&sid,&inval ) < 2 )
			{
				cout << i <<"\t------------------------"<< endl;		
				cerr << "!!!!!!!!! пара SensorId=State #"<<i<<" '"<<arg<<"' задана неверно!!!!!!\n"<< endl;
				err=1;
				continue;
			}
		}
		else
		{
			int ind;
			string strval;
			ind = arg.find_first_of("=");
			string name(arg.substr(0,ind));
			strval = arg.substr( ind + 1, arg.length() );
			sid = conf->getSensorID(name);

			if( sid == UniSetTypes::DefaultObjectId || (sscanf( strval.c_str(),"%ld",&inval ) < 1) )
			{
				cout << i <<"\t------------------------"<< endl;
				cerr << "!!!!!!!!! пара SensorName=State #"<<i<<" '"<<arg<<"' задана неверно!!!!!!\n"<< endl;
				err=1;
				continue;
			}
		}
		
		if(inval==1)
			val=true;
		else if(inval==0)
			val=false;
		else
		{
			cout << i <<"\t------------------------"<< endl;
			cerr << "!!!!!!!!! State в паре SensorId=State #"<<i<<" '"<<arg<<"' должен быть булевым !!!!!!\n"<< endl;
			err=1;
			continue;
		}		

		cout << i <<"\t------------------------"<< endl;		
		try
		{		
//			cout <<"\n\t"<<sid<<"\t"<<val<<endl;		
			cout << "  state: " << val << endl;
			cout << "   name: (" << sid << ") " << conf->oind->getMapName(sid) << endl;
			cout << "   text: " << conf->oind->getTextName(sid) << "\n\n";
			ui.setState(sid,val);
		}
		catch(Exception& ex)
		{
			unideb[Debug::CRIT] << "(setState): " << ex << endl;;
			err = 1;
		}	
	}
	return err;
}

// --------------------------------------------------------------------------------------
int getState(string args, UniversalInterface &ui)
{
	int err;
	err=0;
	string arg;
	ostringstream vout;
	vout<<"-----------------\n| ID\t| State\t|\n-----------------\n";
	UniSetTypes::ObjectId sid(DefaultObjectId);
	int state;

	cout << " getState ==============================\n";
	for(int i=1;separateArgs(args,arg);i++)
	{
		if( isdigit( arg[0] ) )
		{
			if( sscanf( arg.c_str(),"%ld",&sid ) < 1 )
			{
				cout << i <<"\t------------------------"<< endl;
				cerr << "!!!!!!!!  SensorID #"<<i<<" '"<<arg<<"' задан неверно!!!!!!!\n"<< endl;
				err = 1;
				vout<<"| "<<arg<<"\t| ?\t| SensorID задан неверно !!!\n-----------------\n";
				continue;
			}
		}	
		else
		{
			sid = conf->getSensorID(arg);

			if( sid == UniSetTypes::DefaultObjectId )
			{
				cout << i <<"\t------------------------"<< endl;		
				cerr << "!!!!!!!!!  SensorName #"<<i<<" '"<<arg<<"' задан неверно!!!!!!\n"<< endl;
				err=1;
				vout<<"| "<<arg<<"\t| ?\t| SensorName задан неверно !!!\n-----------------\n";				
				continue;
			}
		}

		cout << i <<"\t-----------------------"<< endl;
		try
		{
			cout << "    name: (" << sid << ") " << conf->oind->getMapName(sid) << endl;
			cout << "    text: " << conf->oind->getTextName(sid) << endl;
			state = ui.getState(sid);
			cout << "   state: " <<state << "\n\n";
			vout<<"| "<<arg<<"\t| "<<state<<"\t|\n-----------------\n";
		}
		catch(Exception& ex)
		{
			cout <<"!!!!!!!!! err: " << ex << endl<<endl;
			vout<<"| "<<arg<<"\t| ?\t| "<<ex<<" !!!\n-----------------\n";
			err = 1;
		}
	}
	
	if( !err )
		cout << vout.str() << endl;
	
	return err;
}

// --------------------------------------------------------------------------------------
int getValue(string args, UniversalInterface &ui )
{
	int err;
	long value;
	err=0;
	string arg;
	ostringstream vout;
	vout<<"-----------------\n| ID\t| Value\t|\n-----------------\n";
	UniSetTypes::ObjectId sid(DefaultObjectId);

	cout << "\n getValue ==============================\n";
	for(int i=1;separateArgs(args,arg);i++)
	{
		if( isdigit( arg[0] ) )
		{
			if( sscanf( arg.c_str(),"%ld",&sid ) < 1 )
			{
				cout << i <<"\t------------------------"<< endl;
				cerr << "!!!!!!!!  SensorID #"<<i<<" '"<<arg<<"' задан неверно!!!!!!!\n"<< endl;
				err = 1;
				vout<<"| "<<arg<<"\t| ?\t| SensorID задан неверно !!!\n-----------------\n";
				continue;
			}
		}	
		else
		{
			sid = conf->getSensorID(arg);

			if( sid == UniSetTypes::DefaultObjectId )
			{
				cout << i <<"\t------------------------"<< endl;		
				cerr << "!!!!!!!!!  SensorName #"<<i<<" '"<<arg<<"' задан неверно!!!!!!\n"<< endl;
				err=1;
				vout<<"| "<<arg<<"\t| ?\t| SensorName задан неверно !!!\n-----------------\n";				
				continue;
			}
		}

		cout << i <<"\t-----------------------"<< endl;
		try
		{
			cout << "    name: (" << sid << ") " << conf->oind->getMapName(sid) << endl;
			cout << "    text: " << conf->oind->getTextName(sid) << endl;
			value = ui.getValue(sid);
			cout << "   value: " << value << "\n\n";
			vout<<"| "<<arg<<"\t| "<< value <<"\t|\n-----------------\n";
		}
		catch(Exception& ex)
		{
			unideb[Debug::CRIT] << "(getValue): " << ex << endl;;
			err = 1;
		}
	}
	
	cout <<vout.str()<<endl;
	return err;
}
// --------------------------------------------------------------------------------------
int getCalibrate(string arg, UniversalInterface &ui)
{
	UniSetTypes::ObjectId sid(uni_atoi(arg));
	if( sid <= 0 )
	{
		cout << "(getCalibrate): Не задан SensorId аналогового датчика!!!!!!"<< endl;
		return 1;
	}

	cout << "getCalibrate --------\n";
	cout << "      name: (" << sid << ") " << conf->oind->getMapName(sid) << endl;
	cout << "      text: " << conf->oind->getTextName(sid) << "\n";
	cout << "калибровка: ";
	IOController_i::SensorInfo si;
	si.id = sid;
	si.node = conf->getLocalNode();
	IOController_i::CalibrateInfo ci = ui.getCalibrateInfo(si);
	cout << "rmin=" << ci.minRaw << " rmax=" << ci.maxRaw;
	cout << " cmin=" << ci.minCal << " cmax=" << ci.maxCal;
	cout << " sensibility=" << ci.sensibility;
	cout << endl;

	return 0;
}

// --------------------------------------------------------------------------------------
int getRawValue(string arg, UniversalInterface &ui )
{
	UniSetTypes::ObjectId sid(uni_atoi(arg));
	if( sid==0 )
	{
		cout << "(getRawValue): Не задан SensorId аналогового датчика!!!!!!"<< endl;
		return 1;
	}
	IOController_i::SensorInfo si;
	si.id = sid;
	si.node = conf->getLocalNode();
	cout << "getRawValue --------\n";
	cout << "   name: (" << sid << ") " << conf->oind->getMapName(sid) << endl;
	cout << "   text: " << conf->oind->getTextName(sid) << "\n\n";
	cout << "  value: " << ui.getRawValue(si) << endl;

	return 0;
}

// --------------------------------------------------------------------------------------
int logRotate(string arg, UniversalInterface &ui )
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
int configure(string arg, UniversalInterface &ui )
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
int oinfo(string arg, UniversalInterface &ui )
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
/*
bool getID( const string arg, ObjectId id, ObjectId node )
{
		if( isdigit( arg[0] ) )
		{
			if( sscanf( arg.c_str(),"%ld",&id ) < 1 )
			{
				cout << i <<"\t------------------------"<< endl;
				cerr << "!!!!!!!!  SensorID #"<<i<<" '"<<arg<<"' задан неверно!!!!!!!\n"<< endl;
				err = 1;
				vout<<"| "<<arg<<"\t| ?\t| SensorID задан неверно !!!\n-----------------\n";
				continue;
			}
		}	


	string::size_type pos = arg.find_first_of(":");
	if( pos == string::npos )
	{
		id = uni_atoi(arg);
		return true;
	}

	id = uni_atoi(name.substr(0,pos-1));
	node = uni_atoi( name.substr(pos+1,name.size()) );
	return true;
}
*/
// --------------------------------------------------------------------------------------
