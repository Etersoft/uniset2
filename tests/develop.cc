#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <tuple>
#include "UTCPCore.h"

template<typename... Args>
class VMon
{
	public:

		std::tuple<std::unordered_map<const Args*, const std::string>...> m_tuple;
};

// ------------------------------------------------------------------------------
using namespace std;

int main( int argc, const char** argv )
{
	unsigned char dat[] = { '1', '2', '3' , '4' };

	//	UTCPCore::Buffer*  buf = new UTCPCore::Buffer( dat, 0 );
	UTCPCore::Buffer*  buf = new UTCPCore::Buffer( dat, 3 );

	//	if( buf->nbytes() == 0 )
	//		delete buf;
	cout << "buf: " << buf->dpos() << endl;


	delete buf;

	//	VMon<int,double,char> vmon;

	//	cout << std::get<0>(vmon.m_tuple).size() << endl;

	return 0;
}
