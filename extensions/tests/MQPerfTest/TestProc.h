// -----------------------------------------------------------------------------
#ifndef TestProc_H_
#define TestProc_H_
// -----------------------------------------------------------------------------
#include "TestProc_SK.h"
// -----------------------------------------------------------------------------
class TestProc:
	public TestProc_SK
{
	public:
		TestProc( uniset::ObjectId id, xmlNode* confnode );
		virtual ~TestProc();

		bool isFullQueue();

	protected:
		TestProc();

	private:
};
// -----------------------------------------------------------------------------
#endif // TestProc_H_
// -----------------------------------------------------------------------------
