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
// ------------------------------------------------------------------------------
struct TestClass
{
	TestClass()
	{
		memset(&data,0,sizeof(data));
		cerr << "TEST CLASS CREATE.." << endl;
	}

	//	TestClass( TestClass&& ) = default;
	TestClass( const TestClass& t )
	{
		cerr << "TEST CLASS COPY.." << endl;
		(*this) = t;
	}

	TestClass& operator=(const TestClass&  t )
	{
		cerr << "TEST CLASS COPY FUNC.." << endl;
		(*this) = t;
		return (*this);
	}

	TestClass& operator=( TestClass&& t ) = default;

	TestClass( TestClass&& t )
	{
		cerr << "TEST CLASS MOVE.." << endl;
		(*this) = std::move(t);
	}

	size_t len = { 10 };
	int data[10];
};

struct MClass
{
	MClass( int d1, int d2 = 0 )
	{
		data[0] = d1;
		data[1] = d2;
	}

	size_t len = { 2 };
	int data[2];

	TestClass get()
	{
		TestClass m;
		m.len = len;
		memcpy(data, &m.data, sizeof(data));
		//return std::move(m);
		return m;
	}
};

void test_func( TestClass& m )
{
	cerr << "func.." << endl;
}

void test_func( TestClass&& m )
{
	cerr << "move func.." << endl;
}

// ------------------------------------------------------------------------------

int main( int argc, const char** argv )
{
	MClass m(10, 11);

	TestClass c = m.get();

	test_func(c);
	test_func( std::move(c) );

	return 0;
}
