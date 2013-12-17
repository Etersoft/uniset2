/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman
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
// --------------------------------------------------------------------------
/*! \file
 *  \author Pavel Vainerman
*/
// -------------------------------------------------------------------------- 
#include <omniORB4/CORBA.h>
#include <omniORB4/omniURI.h>
#include <sstream>
#include "ORepHelpers.h"
#include "UniSetTypes.h"
#include "Configuration.h"
#include "Debug.h"

using namespace omni;
using namespace UniSetTypes;
using namespace std;

namespace ORepHelpers
{
	

// --------------------------------------------------------------------------
    /*!
     *	\param cname - полное имя контекста ссылку на который, возвратит функция. 
     *	\param argc, argv  - параметры инициализации ORB      
    */
	CosNaming::NamingContext_ptr getContext(const string& cname, int argc, const char* const* argv, const string& nsName )throw(ORepFailed)
    {
		CORBA::ORB_var orb = CORBA::ORB_init( argc, (char**)argv );
		if( ulog.is_repository() )
			ulog.repository() << "OREPHELP: orb init ok"<< endl;
		return getContext(orb, cname, nsName);
	}
// --------------------------------------------------------------------------
	CosNaming::NamingContext_ptr getContext(CORBA::ORB_ptr orb, const string& cname,  const string& servname)throw(ORepFailed)
	{
        CosNaming::NamingContext_var rootC;

		if( ulog.is_repository() )
			ulog.repository() << "OREPHELPER(getContext): get rootcontext...(servname = "<< servname << ")" <<endl;

		rootC = getRootNamingContext(orb, servname);

		if( ulog.is_repository() )
			ulog.repository() << "OREPHELPER(getContext): get rootContect ok " << endl;

        if( CORBA::is_nil(rootC) )
		{
			if( ulog.is_warn() )
				ulog.warn() << "OREPHELPER: не смог получить ссылку на NameServices"<< endl;
			throw ORepFailed("OREPHELPER(getContext): не смог получить ссылку на NameServices");
        }

		if ( cname.empty() )
			return rootC._retn();

		if( ulog.is_repository() )
			ulog.repository() << "OREPHELPER(getContext): get ref context " << cname << endl;

		CosNaming::Name_var ctxName = omniURI::stringToName(cname.c_str());
        CosNaming::NamingContext_var ctx;
        try
		{
			CORBA::Object_var o = rootC->resolve(ctxName);
			ctx = CosNaming::NamingContext::_narrow(o);
			if( CORBA::is_nil(ctx) )
			{
	    		const string err("OREPHELPER(getContext): не смог получить ссылку на контекст(is_nil) "+cname);
			    throw ORepFailed(err.c_str());
			}
        }
		catch(const CosNaming::NamingContext::InvalidName &nf)
        {
			ostringstream err;
	    	err << "OREPHELPER(getContext): не смог получить ссылку на контекст " << cname;
			if( ulog.is_warn() )
			ulog.warn() << err.str() << endl;
		    throw ORepFailed(err.str());
        }
		catch(const CosNaming::NamingContext::NotFound &nf)
        {
			ostringstream err;
			err << "OREPHELPER(getContext): не найден контекст " << cname;
			if( ulog.warn() )
				ulog.warn() << err.str() << endl;
			throw ORepFailed(err.str());
        }
		catch(const CosNaming::NamingContext::CannotProceed &np)
        {
			ostringstream err;
			err << "OREPHELPER(getContext): catch CannotProced " << cname;
			err << " bad part=" << omniURI::nameToString(np.rest_of_name);
			if( ulog.is_warn() )
				ulog.warn() << err.str() << endl;
			throw ORepFailed(err.str());
        }
	    catch(CORBA::SystemException& ex)
	    {
			ostringstream err;
			err << "OREPHELPER(getContext): поймали CORBA::SystemException: " << ex.NP_minorString();
			if( ulog.is_warn() )
				ulog.warn() <<  err.str() << endl;
			throw ORepFailed(err.str());
	    }	
	    catch(CORBA::Exception&)
    	{
			if( ulog.is_warn() )
				ulog.warn() << "OREPHELPER(getContext): поймали CORBA::Exception." << endl;
			throw ORepFailed();
	    }
    	catch(omniORB::fatalException& fe)
	    {
			ostringstream err;
			err << "OREPHELPER(getContext): поймали omniORB::fatalException:";
			if( ulog.is_warn() )
			{
				ulog.warn() <<  err << endl;
			ulog.warn() << "  file: " << fe.file() << endl;
				ulog.warn() << "  line: " << fe.line() << endl;
		ulog.warn() << "  mesg: " << fe.errmsg() << endl;
			}
			throw ORepFailed(err.str());
	    }

		if( ulog.is_repository() )
			ulog.repository() << "getContext: получили "<< cname << endl;

		// Если _var
// 	 	return CosNaming::NamingContext::_duplicate(ctx);
		return ctx._retn();

		// Если _ptr
//		return ctx;
	}

    // ---------------------------------------------------------------------------------------------------------------
    /*!	\param orb - ссылка на ORB */
    CosNaming::NamingContext_ptr getRootNamingContext(CORBA::ORB_ptr orb, const string& nsName, int timeoutSec)
    {
		CosNaming::NamingContext_var rootContext;
	try
	{
//		cout << "ORepHelpers(getRootNamingContext): nsName->" << nsName << endl;
		CORBA::Object_var initServ = orb->resolve_initial_references(nsName.c_str());
		if( ulog.is_repository() )
			ulog.repository() << "OREPHELP: get rootcontext...(nsName = "<< nsName << ")" <<endl;

		rootContext = CosNaming::NamingContext::_narrow(initServ);
		if (CORBA::is_nil(rootContext))
		{
    		string err("ORepHelpers: Не удалось преобразовать ссылку к нужному типу.");
			throw ORepFailed(err.c_str());
		}
		
		if( ulog.is_repository() )
			ulog.repository() << "OREPHELP: init NameService ok"<< endl;
	}
	catch(CORBA::ORB::InvalidName& ex)
	{
		ostringstream err;
		err << "ORepHelpers(getRootNamingContext): InvalidName=" << nsName;
		if( ulog.is_warn() )
			ulog.warn() << err.str() << endl;
		throw ORepFailed(err.str());
	}
	catch (CORBA::COMM_FAILURE& ex)
	{
		ostringstream err;
		err << "ORepHelpers(getRootNamingContext): Не смог получить ссылку на контекст ->" << nsName;
		throw ORepFailed(err.str());
	}
	catch(omniORB::fatalException& ex)
	{
    	string err("ORepHelpers(getRootNamingContext): Caught Fatal Exception");
		throw ORepFailed(err);
	}
	catch (...)
	{
    	string err("ORepHelpers(getRootNamingContext): Caught a system exception while resolving the naming service.");
		throw ORepFailed(err);
	}

	if( ulog.is_repository() )
		ulog.repository() << "OREPHELP: get root context ok"<< endl;

//	// Если создан как _ptr
//	return rootContext;

//	Если создан как _var
	return rootContext._retn();
    }
    // ---------------------------------------------------------------------------------------------------------------
    /*!
     *	\param fname - полное имя включающее в себя путь ("Root/Section1/name|Node:Alias")
     *	\param brk  - используемый символ разделитель      
    */
    const string getShortName( const string& fname, const std::string& brk )
    {
/*    
		string::size_type pos = fname.rfind(brk);
		if( pos == string::npos )
			return fname;

		return fname.substr( pos+1, fname.length() );
*/
		string::size_type pos1 = fname.rfind(brk);
		string::size_type pos2 = fname.rfind(conf->oind->sepName);

		if( pos2 == string::npos && pos1 == string::npos )
			return fname;
	
		if( pos1 == string::npos )
			return fname.substr( 0, pos2 );
	
		if( pos2 == string::npos )
			return fname.substr( pos1+1, fname.length() );

			return fname.substr( pos1+1, pos2-pos1-1 );
	}

    // ---------------------------------------------------------------------------------------------------------------
    /*!
     *	\param fullName - полное имя включающее в себя путь
     *	\param brk  - используемый символ разделитель      
     *  \note Функция возвращает путь без последнего символа разделителя ("Root/Section1/name" -> "Root/Section1")
    */
    const string getSectionName( const string& fullName, const std::string& brk )
    {
		string::size_type pos = fullName.rfind(brk);
		if( pos == string::npos )
			return "";

		return fullName.substr(0, pos);
    }	

    // ---------------------------------------------------------------------------------------------------------------
	/*
	 *	Запрещенные символы см. UniSetTypes::BadSymbols[]
	 *	\return Если не найдено запрещенных символов то будет возвращен 0, иначе найденный символ
	*/
    char checkBadSymbols(const string& str)
    {
		using namespace UniSetTypes;
		
		for ( unsigned int i=0;i<str.length();i++)
		{
			for(unsigned int k=0; k<sizeof(BadSymbols); k++)
			{
				if ( str[i]==BadSymbols[k] )
					return (char)BadSymbols[k];
			}
		}
		return 0;
    }	

    // ---------------------------------------------------------------------------------------------------------------
	string BadSymbolsToStr()
	{
		string bad="";
		for (unsigned int i=0; i<sizeof(UniSetTypes::BadSymbols); i++)
		{
			bad+="'";
			bad+=UniSetTypes::BadSymbols[i];
			bad+="', ";

		}
		string err("Имя не должно содержать символы: "+ bad);
		return err;
	}
    // ---------------------------------------------------------------------------------------------------------------
}

