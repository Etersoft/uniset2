//--------------------------------------------------------------------------------
#ifndef _SMVIEWER_H
#define _SMVIEWER_H
//--------------------------------------------------------------------------------
#include <string>
#include "SViewer.h"
#include "SMInterface.h"
//--------------------------------------------------------------------------------
class SMViewer:
	public SViewer
{   
	public:
		SMViewer( UniSetTypes::ObjectId shmID );
		virtual ~SMViewer();

		void run();
		
	protected:

	private:
		SMInterface* shm;
		UInterface ui;
};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
