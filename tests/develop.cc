#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <tuple>

template<typename... Args>
class VMon
{
	public:

		std::tuple<std::unordered_map<const Args*,const std::string>...> m_tuple;
};

// ------------------------------------------------------------------------------
using namespace std;

int main( int argc, const char** argv )
{
//	VMon<int,double,char> vmon;

//	cout << std::get<0>(vmon.m_tuple).size() << endl;

	return 0;
}
