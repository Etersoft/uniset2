// $Id: main.cc,v 1.1 2009/03/03 12:27:37 pv Exp $
#include <iostream>
#include "Exceptions.h"
#include "UniversalInterface.h"

// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
void help_print()
{
	cout << endl << "--help        - Помощь по утилите" << endl;
	cout << "--sid id              - sensor ID (AnalogInput)" << endl;
	cout << "--min val             - Нижняя граница датчика. По умолчанию 0" << endl;
	cout << "--max val             - Верхняя граница датчика. По умолчанию 100 " << endl;
	cout << "--step val            - Шаг датчика. По умолчанию 1" << endl;
	cout << "--pause msec          - Пауза. По умолчанию 200 мсек" << endl << endl;                                                           
}

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
		UniversalInterface ui;

		int asid = conf->getArgInt("--sid","");
		if( asid<=0 )
		{
			cerr << endl << "Use --sid id" << endl << endl;
			return 1;
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
		cout << "  sid = " << asid << endl;
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
				ui.saveValue(asid, j, UniversalIO::AnalogInput);
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
				ui.saveValue(asid, i,UniversalIO::AnalogInput);
		    }
		    msleep(amsec);
		}

	}
	catch( Exception& ex )
	{
		cerr << endl << "(main): " << ex << endl;
		return 1;
	}
	catch( ... )
	{
		cerr << endl << "catch..." << endl;
		return 1;
	}
	
	return 0;
}
// ------------------------------------------------------------------------------------------
