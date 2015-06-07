// --------------------------------------------------------------------------
#include <string>
#include <sstream>
#include <iomanip>
#include "VMonitor.h"
// --------------------------------------------------------------------------
#define VMON_IMPL_ADD(T) void VMonitor::add( const std::string& name, const T& v ) \
{\
	m_##T.emplace(&v,name); \
}
// --------------------------------------------------------------------------
#define VMON_IMPL_ADD2(T) \
void VMonitor::add( const std::string& name, const T& v ) \
{\
	m_##T.emplace(&v,name); \
} \
void VMonitor::add( const std::string& name, const unsigned T& v ) \
{\
	m_unsigned_##T.emplace(&v,name); \
}
// --------------------------------------------------------------------------
#define VMON_IMPL_PRN(M,T) \
{\
	for( const auto& e: M.m_##T ) \
	   os << e.second << "=" << *(e.first) << std::endl;\
}
// --------------------------------------------------------------------------
#define VMON_IMPL_PRN2(M,T) \
{\
	for( const auto& e: M.m_##T ) \
	   os << e.second << "=" << *(e.first) << std::endl;\
\
	for( const auto& e: M.m_unsigned_##T ) \
	   os << e.second << "=" << *(e.first) << std::endl;\
}
// --------------------------------------------------------------------------
#define VMON_IMPL_PRET(T) \
{\
	for( const auto& e: m_##T ) \
	   os << std::right << std::setw(15) << e.second << std::left << " = " << std::right << std::setw(6) << *(e.first) << std::endl;\
}
// --------------------------------------------------------------------------
#define VMON_IMPL_PRET2(T) \
{\
	for( const auto& e: m_##T ) \
	   os << std::right << std::setw(15) << e.second << std::left << " = " << std::right << std::setw(6) << *(e.first) << std::endl;\
\
	for( const auto& e: m_unsigned_##T ) \
	   os << std::right << std::setw(15) << e.second << std::left << " = " << std::right << std::setw(6) << *(e.first) << std::endl;\
}
// --------------------------------------------------------------------------
VMON_IMPL_ADD2(int)
VMON_IMPL_ADD2(long)
VMON_IMPL_ADD2(short)
VMON_IMPL_ADD(bool)
VMON_IMPL_ADD(float)
VMON_IMPL_ADD(double)
// --------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, VMonitor& m )
{
	VMON_IMPL_PRN2(m,int);
	VMON_IMPL_PRN2(m,long);
	VMON_IMPL_PRN2(m,short);
	VMON_IMPL_PRN(m,bool);
	VMON_IMPL_PRN(m,float);
	VMON_IMPL_PRN(m,double);

	return os;
}

// --------------------------------------------------------------------------
std::string VMonitor::str()
{
	std::ostringstream s;
	s << (*this);
	return std::move(s.str());
}
// --------------------------------------------------------------------------
std::string VMonitor::pretty_str()
{
	std::ostringstream os;
	VMON_IMPL_PRET2(int);
	VMON_IMPL_PRET2(long);
	VMON_IMPL_PRET2(short);
	VMON_IMPL_PRET(bool);
	VMON_IMPL_PRET(float);
	VMON_IMPL_PRET(double);

	return std::move(os.str());
}
// --------------------------------------------------------------------------
