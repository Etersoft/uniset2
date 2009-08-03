// $Id: main.cc,v 1.9 2008/02/10 03:21:42 vpashka Exp $
// --------------------------------------------------------------------------
#include <string>
#include "SViewer.h"
#include "Configuration.h"
// --------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
// --------------------------------------------------------------------------
static void short_usage()
{
	cout << "Usage: uniset-sviewer-text [--fullname] [--polltime msec] [--confile uniset-confile]\n";
}
// --------------------------------------------------------------------------
int main(int argc, const char **argv)
{   
	try
	{
		if( argc > 1 && !strcmp(argv[1],"--help") )
		{
			short_usage();
			return 0;
		}

		uniset_init(argc,argv,"configure.xml");
/*
		UniversalInterface ui;
		IDList lst;
		lst.add(1);
		lst.add(2);
		lst.add(3);
		lst.add(5);
		lst.add(33);
		IOController_i::ASensorInfoSeq_var seq = ui.getSensorSeq(lst);
		int size = seq->length();
		for(int i=0; i<size; i++)
			cout << "id=" << seq[i].si.id << " val=" << seq[i].value << endl;
*/			
		bool fullname = atoi(conf->getArgParam("--fullname").c_str());

		SViewer sv(conf->getControllersSection(),!fullname);
		int timeMS = atoi(conf->getArgParam("--polltime").c_str());
		if( timeMS )
		{
			cout << "(main): просматриваем с периодом " << timeMS << "[мсек]" <<  endl;
			sv.monitor(timeMS);
		}
		else
			sv.view();
	}
	catch(Exception& ex )
    {
		cerr << "(main): Поймали исключение " << ex <<  endl;
    }
    catch(...)
    {
    	cerr << "(main): Неизвестное исключение!!!!"<< endl;
    }

	return 0;
}
