/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
//--------------------------------------------------------------------------------
/*! \file
 *  \brief Программа просмотра состояния датчиков
 *  \author Pavel Vainerman
 */
//--------------------------------------------------------------------------------
#ifndef _SVIEWER_H
#define _SVIEWER_H
//--------------------------------------------------------------------------------
#include <string>
#include <memory>
#include "IOController_i.hh"
#include "ObjectRepository.h"
#include "UInterface.h"
#include "PassiveTimer.h"
//--------------------------------------------------------------------------------
class SViewer
{
	public:

		explicit SViewer(const std::string& ControllersSection, bool isShortName = true);
		virtual ~SViewer();

		void view();
		void monitor( timeout_t timeoutMS = 500 );

	protected:
		void readSection(const std::string& sec, const std::string& secRoot);
		void getInfo(UniSetTypes::ObjectId id);

		virtual void updateSensors( IOController_i::SensorInfoSeq_var& amap, UniSetTypes::ObjectId oid );
		virtual void updateThresholds( IONotifyController_i::ThresholdsListSeq_var& tlst, UniSetTypes::ObjectId oid );

		const std::string csec;
		void printInfo(UniSetTypes::ObjectId id, const std::string& sname, long value, const std::string& supplier,
					   const std::string& txtname, const std::string& iotype);

		std::shared_ptr<UInterface> ui;

	private:
		ObjectRepository rep;
		bool isShortName = { true };

};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
