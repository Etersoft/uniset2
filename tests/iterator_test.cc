#include <iostream>
#include <algorithm>

using namespace std;

#include "Exceptions.h"
#include "UniXML.h"
#include "UniSetTypes.h"


void check( const std::string& name, const std::string& true_res, const std::string& your_res )
{
	cout<<name<<endl;
	cout<<"Correct result: "<<true_res<<endl;

	if( your_res == true_res )
	cout<<"Your result: "<<your_res<<"\n"<<endl;
	else
	cout<<"Your result - ERROR!"<<"\n"<<endl;
}


void myf(xmlNode it)
{
	cout<<it.name<<endl;
}

int main()
{
		UniXML xml("iterator_test.xml");
		UniXML::iterator it=xml.begin();

		it.find("messages");
		check( "Check find():", "messages", it.getName() );

		it=xml.begin();
		it.find("d7");
		check( "Check find():", "d7", it.getName() );

		it=xml.begin();
		it.findName("c5","6abc");
		check( "Check findName():", "c5", it.getName() );

		it=xml.begin();
		it.findName("messages","messages");
		check( "Check findName():", "messages", it.getName() );

		it=xml.begin();
		it.find("UniSet");
		it++;
		++it;
		check( "Check iterator ++ :", "ObjectsMap", it.getName() );

		it=xml.begin();
		it.find("testII");
		it++;
		it--;
		--it;
		check( "Check iterator -- :", "a2", it.getName() );


		it=xml.begin();
		it.goChildren();
		cout<<"Check algorythm 'for_each()':\n";
		for_each(it,xml.end(),myf);

		cout<<"\nCorrect result for algoryhtm 'for_each()':\n"<<"UniSet \n"<<"dlog \n"
		<<"ObjectsMap \n"<<"messages \n";


		it=xml.begin();
		it.find("test");
		if( it.find("messages") )
			cout << "ERROR! begin=<UniSet> but find <messages>!" << endl;


	return 0;
}
