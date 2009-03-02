/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman <pv>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
//--------------------------------------------------------------------------------
/*! \file
 *  \brief Программа просмотра состояния датчиков
 *  \author Павел Вайнерман <pv>
 *  \date   $Date: 2007/06/17 21:30:55 $
 *  \version $Id: SViewer.h,v 1.11 2007/06/17 21:30:55 vpashka Exp $
 *  \par
 */
//--------------------------------------------------------------------------------
#ifndef _SVIEWER_H
#define _SVIEWER_H
//--------------------------------------------------------------------------------
#include <string>
#include "IOController_i.hh"
#include "ObjectRepository.h"
#include "UniversalInterface.h"
//--------------------------------------------------------------------------------
class SViewer 
{   
	public:

		SViewer(const std::string ControllersSection, bool isShort=true);
		virtual ~SViewer();

		void view();
		void monitor( int timeoutMS=500 );
		
	protected:
        friend class SViewer_glade;
		void on_SViewer_destroy();

		void readSection(const std::string sec, const std::string secRoot);
		void getInfo(UniSetTypes::ObjectId id);

		virtual void updateDSensors(IOController_i::DSensorInfoSeq_var& dmap, UniSetTypes::ObjectId oid);
		virtual void updateASensors(IOController_i::ASensorInfoSeq_var& amap, UniSetTypes::ObjectId oid);
		virtual void updateThresholds(IONotifyController_i::ThresholdsListSeq_var& tlst, UniSetTypes::ObjectId oid);

		const std::string csec;
		void printInfo(UniSetTypes::ObjectId id, const std::string& sname, long value, const std::string& owner, 
						const std::string& txtname, const std::string& iotype);

	private:
		ObjectRepository rep;
		UniversalInterface::CacheOfResolve cache;
		UniversalInterface ui;
		bool isShort;

};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
