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
// -------------------------------------------------------------------------
#ifndef UConnector_H_
#define UConnector_H_
// --------------------------------------------------------------------------
#include <memory>
#include <string>
#include "Configuration.h"
#include "UInterface.h"
#include "UTypes.h"
#include "UExceptions.h"
#include "UniSetActivator.h"
// --------------------------------------------------------------------------
class UConnector
{
	public:
		UConnector( int argc, char** argv, const std::string& xmlfile )throw(UException);
		UConnector( UTypes::Params* p, const std::string& xmlfile )throw(UException);
		~UConnector();

		inline std::string getUIType()
		{
			return string("uniset");
		}

		std::string getConfFileName();
		long getValue( long id, long node )throw(UException);
		void setValue( long id, long val, long node, long supplier = UTypes::DefaultSupplerID )throw(UException);

		long getSensorID( const std::string& name );
		long getNodeID( const std::string& name );
		long getObjectID( const std::string& name );

		std::string getShortName( long id );
		std::string getName( long id );
		std::string getTextName( long id );

		void activate_objects() throw(UException);

	private:
		std::shared_ptr<UniSetTypes::Configuration> conf;
		std::shared_ptr<UInterface> ui;
		std::string xmlfile;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
