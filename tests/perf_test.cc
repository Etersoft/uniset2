#include <map>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <iomanip>

#include "Configuration.h"

using namespace std;

std::map< uniset::ObjectId, int > std_m;
std::unordered_map< uniset::ObjectId, int > std_un;
std::vector< uniset::ObjectId > values;

const int N = 10000000;
int main( int argc, char* argv[] )
{
	auto conf = uniset::uniset_init(argc, argv);

	auto ind = conf->oind;

	int t1, t2;

	for ( int i = 0; i < N; i++ )
		values.emplace_back( rand() * rand() );

#if 0
	std::cout << "insert:" << std::endl;

	for ( int k = 0; k < 5; k++ )
	{
		t1 = clock();

		for ( int i = 0; i < N; i++ )
			std_m[ values[ i ] ] = i;

		t2 = clock();
		std::cout << "std   : " << std::setw( 8 ) << (t2 - t1) / (double)CLOCKS_PER_SEC * 1000 << " ms." << std::endl;
	}

	for ( int k = 0; k < 5; k++ )
	{
		t1 = clock();

		for ( int i = 0; i < N; i++ )
			std_un[ values[ i ] ] = i;

		t2 = clock();
		std::cout << "std_un: " << std::setw( 8 ) << (t2 - t1) / (double)CLOCKS_PER_SEC * 1000 << " ms." << std::endl;
	}

#endif

	std::cout << "find:" << std::endl;

	{
		t1 = clock();

		for ( int k = 0; k < N; k++ )
		{
			auto v = std_m.find( rand() );
		}

		t2 = clock();
		std::cout << "std   : " << std::setw( 8 ) << (t2 - t1) / (double)CLOCKS_PER_SEC * 1000 << " ms." << std::endl;
	}

	{
		t1 = clock();

		for ( int k = 0; k < N; k++ )
		{
			auto v = std_un.find( rand() );
		}

		t2 = clock();
		std::cout << "std_un: " << std::setw( 8 ) << (t2 - t1) / (double)CLOCKS_PER_SEC * 1000 << " ms." << std::endl;
	}

	return 0;
}
