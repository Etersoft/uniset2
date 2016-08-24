#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <tuple>
#include "UTCPCore.h"
#include <chrono>

template<typename... Args>
class VMon
{
	public:

		std::tuple<std::unordered_map<const Args*, const std::string>...> m_tuple;
};

// ------------------------------------------------------------------------------
using namespace std;


class PtrMapHashFn
{
	public:
		size_t operator() (const long* const& key) const
		{
			return std::hash<long>()((long)key);
		}
};


int main( int argc, const char** argv )
{
	unordered_map<const long*, const long*,PtrMapHashFn> vmap;

	const long id = 10;
	long prive_val = 100;
	const long& val(prive_val);

	vmap.emplace(&id,&val);


	auto i = vmap.find(&id);
	return 0;



	auto now = std::chrono::system_clock::now();
	auto sec = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
	auto nsec = std::chrono::time_point_cast<std::chrono::seconds>(now);

	cout << "SEC=" << std::chrono::duration<double>(sec.time_since_epoch()).count()
		 << endl;
	return 0;


	std::chrono::time_point<std::chrono::system_clock> p1, p2, p3;

	p2 = std::chrono::system_clock::now();
	p3 = p2 - std::chrono::hours(24);

	std::time_t epoch_time = std::chrono::system_clock::to_time_t(p1);
	std::cout << "epoch: " << std::ctime(&epoch_time);
	std::time_t today_time = std::chrono::system_clock::to_time_t(p2);
	std::cout << "today: " << std::ctime(&today_time);

	std::cout << "hours since epoch: "
			  << std::chrono::duration_cast<std::chrono::hours>(
				  p2.time_since_epoch()).count()
			  << '\n';
	std::cout << "yesterday, hours since epoch: "
			  << std::chrono::duration_cast<std::chrono::hours>(
				  p3.time_since_epoch()).count()
			  << '\n';

	return 0;
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
