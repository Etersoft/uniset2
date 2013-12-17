#include <iostream>
#include "Exceptions.h"
#include "UInterface.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
void help_print()
{
	cout << endl;
	cout << "--confile configure.xml. По умолчанию: configure.xml." << endl;
	cout << "--sid id1@Node1,id2,..,idXX@NodeXX  - Аналоговые датчики (AI,AO)" << endl;
	cout << endl;
	cout << "--min val       - Нижняя граница датчика. По умолчанию 0" << endl;
	cout << "--max val       - Верхняя граница датчика. По умолчанию 100 " << endl;
	cout << "--step val      - Шаг датчика. По умолчанию 1" << endl;
	cout << "--pause msec    - Пауза. По умолчанию 200 мсек" << endl << endl;
}
// -----------------------------------------------------------------------------
struct ExtInfo:
	public UniSetTypes::ParamSInfo
{
	UniversalIO::IOType iotype;
};
// -----------------------------------------------------------------------------
int main( int argc, char **argv )
{
	try
	{
		// help
		// -------------------------------------
		if( argc>1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
		{
			help_print();
			return 0;
		}	
		// -------------------------------------

		uniset_init(argc, argv, "configure.xml" );
		UInterface ui;
		
		string sid(conf->getArgParam("--sid"));
		if( sid.empty() )
		{
			cerr << endl << "Use --sid id1,..,idXX" << endl << endl;
			return 1;
		}

		std::list<UniSetTypes::ParamSInfo> lst = UniSetTypes::getSInfoList(sid,UniSetTypes::conf);

		if( lst.empty() )
		{
			cerr << endl << "Use --sid id1,..,idXX" << endl << endl;
			return 1;
		}

		std::list<ExtInfo> l;
		for( std::list<UniSetTypes::ParamSInfo>::iterator it = lst.begin(); it!=lst.end(); ++it )
		{
			UniversalIO::IOType t = conf->getIOType( it->si.id );
			if( t != UniversalIO::AI && t != UniversalIO::AO )
			{
				cerr << endl << "Неверный типа датчика '" << t << "' для id='" << it->fname << "'. Тип должен быть AI или AO." << endl << endl;
				return 1;
			}

			if( it->si.node == DefaultObjectId )
				it->si.node = conf->getLocalNode();

			ExtInfo i;
			i.si = it->si;
			i.iotype = t;
			l.push_back(i);
		}

		int amin = conf->getArgInt("--min", "0");
		int amax = conf->getArgInt("--max", "100");
		if( amin>amax )
		{
			int temp = amax;
			amax = amin;
			amin = temp;
		}
		
		int astep = conf->getArgInt("--step", "1");
		if( astep<=0 )
		{
			cerr << endl << "Ошибка, используйте --step val - любое положительное число" << endl << endl;
			return 1;
		}
		
		int amsec = conf->getArgInt("--pause", "200");
		if(amsec<=10)
		{
			cerr << endl << "Ошибка, используйте --pause val - любое положительное число > 10" << endl << endl;
			return 1;
		}
		
		cout << endl << "------------------------------" << endl;
		cout << " Вы ввели следующие параметры:" << endl;
		cout << "------------------------------" << endl;
		cout << "  sid = " << sid << endl;
		cout << "  min = " << amin << endl;
		cout << "  max = " << amax << endl;
		cout << "  step = " << astep << endl;
		cout << "  pause = " << amsec << endl;
		cout << "------------------------------" << endl << endl;
		    					    
		int i = amin-astep, j = amax;


		while(1)
		{
		    if(i>=amax)
		    {
				j -= astep;
					if(j<amin)                 // Принудительная установка нижней границы датчика 
						j = amin;
				cout << "\r" << " i = " << j <<"     "<< flush;

				for( std::list<ExtInfo>::iterator it=l.begin(); it!=l.end(); ++it )
				{
				      try
				      {
							ui.setValue(it->si, j, DefaultObjectId);
				      }
				      catch( Exception& ex )
				      {
						  cerr << endl << "save id="<< it->fname << " " << ex << endl;
				  }
				}

				if(j<=amin)
				{
				    i = amin;
			    	j = amax;
				}
		    }
		    else
		    {
				i += astep;
					if(i>amax)                 // Принудительная установка верхней границы датчика
						i = amax;
			cout << "\r" << " i = " << i <<"     "<< flush;

				for( std::list<ExtInfo>::iterator it=l.begin(); it!=l.end(); ++it )
				{
				      try
				      {
							ui.setValue(it->si, i, DefaultObjectId);
				      }
				      catch( Exception& ex )
				      {
						  cerr << endl << "save id="<< it->fname << " " << ex << endl;
				  }
				}
		    }
		    msleep(amsec);
		}

	}
	catch( Exception& ex )
	{
		cerr << endl << "(simitator): " << ex << endl;
		return 1;
	}
	catch( ... )
	{
		cerr << endl << "(simitator): catch..." << endl;
		return 1;
	}
	
	return 0;
}
// ------------------------------------------------------------------------------------------
