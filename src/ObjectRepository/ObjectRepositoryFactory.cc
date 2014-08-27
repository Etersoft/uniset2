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
#include <string.h>
#include <sstream>
#include "ObjectRepositoryFactory.h"
#include "ORepHelpers.h"
#include "Configuration.h"
#include "Debug.h"

// ---------------------------------------------------------------------------------------------------------------
using namespace UniSetTypes;
using namespace std;
using namespace omni;
// ---------------------------------------------------------------------------------------------------------------
ObjectRepositoryFactory::ObjectRepositoryFactory( Configuration* _conf ):
ObjectRepository(_conf)
{
}

ObjectRepositoryFactory::~ObjectRepositoryFactory()
{
}

// -------------------------------------------------------------------------------------------------------
/*!
 * \param name - имя создаваемой секции
 * \param in_section - полное имя секции внутри которой создается новая
 * \param section - полное имя секции начиная с Root. 
 * \exception ORepFailed - генерируется если произошла при получении доступа к секции
*/
bool ObjectRepositoryFactory::createSection( const char* name, const char* in_section)throw(ORepFailed,InvalidObjectName )
{

    const string str(name);
    char bad = ORepHelpers::checkBadSymbols(str);
    if (bad != 0)
    {
        ostringstream err;
        err << "ObjectRepository(registration): (InvalidObjectName) " << str;
        err << " содержит недопустимый символ " << bad;
        throw ( InvalidObjectName(err.str().c_str()) );
    }

    ulogrep << "CreateSection: name = "<< name << "  in section = " << in_section << endl;
    if( sizeof(in_section)==0 )
    {
        ulogrep << "CreateSection: in_section=0"<< endl;
        return createRootSection(name);
    }

    int argc(uconf->getArgc());
    const char * const* argv(uconf->getArgv());
    CosNaming::NamingContext_var ctx = ORepHelpers::getContext(in_section, argc, argv, uconf->getNSName() );
    return createContext( name, ctx.in() );
}

bool ObjectRepositoryFactory::createSection(const string& name, const string& in_section)throw(ORepFailed,InvalidObjectName)
{
    return createSection(name.c_str(), in_section.c_str());
}

// -------------------------------------------------------------------------------------------------------
/*!
 * \param fullName - полное имя создаваемой секции
 * \exception ORepFailed - генерируется если произошла при получении доступа к секции
*/
bool ObjectRepositoryFactory::createSectionF(const string& fullName)throw(ORepFailed,InvalidObjectName)
{
    string name(ObjectIndex::getBaseName(fullName));
    string sec(ORepHelpers::getSectionName(fullName));

    ulogrep << name << endl;
    ulogrep << sec << endl;

    if ( sec.empty() )
    {
        ulogrep << "SectionName is empty!!!"<< endl;
        ulogrep << "Добавляем в "<< uconf->getRootSection() << endl;
        return createSection(name, uconf->getRootSection());
    }
    else
        return createSection(name, sec);
}

// ---------------------------------------------------------------------------------------------------------------
bool ObjectRepositoryFactory::createRootSection(const char* name)
{
    CORBA::ORB_var orb = uconf->getORB();
    CosNaming::NamingContext_var ctx = ORepHelpers::getRootNamingContext(orb, uconf->getNSName());
    return createContext(name,ctx);
}

bool ObjectRepositoryFactory::createRootSection(const string& name)
{
    return createRootSection( name.c_str() );
}

// -----------------------------------------------------------------------------------------------------------
bool ObjectRepositoryFactory::createContext(const char *cname, CosNaming::NamingContext_ptr ctx)
{
    
    CosNaming::Name_var nc = omniURI::stringToName(cname);
    try
    {
        ulogrep << "ORepFactory(createContext): создаю новый контекст "<< cname << endl;
        ctx->bind_new_context(nc);
        ulogrep << "ORepFactory(createContext): создал. " << endl;
        return true;
    }
    catch(const CosNaming::NamingContext::AlreadyBound &ab)
    {
//        ctx->resolve(nc);
        ulogrep << "ORepFactory(createContext): context "<< cname << " уже есть"<< endl;        
        return true;
    }
    catch(CosNaming::NamingContext::NotFound)
    {
        ulogrep << "ORepFactory(createContext): NotFound "<< cname << endl;        
        throw NameNotFound();
    }
    catch(const CosNaming::NamingContext::InvalidName &nf)
    {
        uwarn << "ORepFactory(createContext): (InvalidName)  " << cname;
    }
    catch(const CosNaming::NamingContext::CannotProceed &cp)
    {
        uwarn << "ORepFactory(createContext): catch CannotProced " 
                << cname << " bad part="
                << omniURI::nameToString(cp.rest_of_name);
        throw NameNotFound();
    }
    catch( CORBA::SystemException& ex)
    {
        ucrit << "ORepFactory(createContext): CORBA::SystemException: "
              << ex.NP_minorString() << endl;
    }
    catch(CORBA::Exception&)
    {
        ucrit << "поймали CORBA::Exception." << endl;
    }
    catch(omniORB::fatalException& fe)
    {
       ucrit << "поймали omniORB::fatalException:" << endl;
       ucrit << "  file: " << fe.file() << endl;
       ucrit << "  line: " << fe.line() << endl;
       ucrit << "  mesg: " << fe.errmsg() << endl;
    }

    return false;
}

// -----------------------------------------------------------------------------------------------------------
/*!
    \note Функция не вывести список, если не сможет получить доступ к секции
*/
void ObjectRepositoryFactory::printSection( const string& fullName )
{
    ListObjectName ls;
    try
    {
        list(fullName.c_str(),&ls);

        if( ls.empty() )
            cout << fullName << " пуст!!!"<< endl;
    }
    catch( ORepFailed )
    {
        cout << "printSection: cath exceptions ORepFailed..."<< endl;
        return ;
    }

    cout << fullName << "(" << ls.size() <<"):" << endl;

    for( auto v: ls )
        cout << v << endl;
}

// -----------------------------------------------------------------------------------------------------------
/*!
 * \param fullName - имя удаляемой секции
 * \param recursive - удлаять рекурсивно все секции или возвращать не удалять и ошибку ( временно )
 * \warning Функция вынимает только первые 1000 объектов, остальные игнорируются...
*/
bool ObjectRepositoryFactory::removeSection(const string& fullName, bool recursive)
{
//    string name = getName(fullName.c_str(),'/');
//  string sec = getSectionName(fullName.c_str(),'/');
//    CosNaming::NamingContext_var ctx = getContext(sec, argc, argv);
    unsigned int how_many = 1000;
    CosNaming::NamingContext_var ctx;
    try
    {    
        int argc(uconf->getArgc());
        const char * const* argv(uconf->getArgv());
        ctx = ORepHelpers::getContext(fullName, argc, argv, nsName);
    }
    catch( ORepFailed )
    {
        return false;
    }

    CosNaming::BindingList_var bl;
    CosNaming::BindingIterator_var bi;

    ctx->list(how_many,bl,bi);


    if( how_many>bl->length() )
        how_many = bl->length();

    bool rem = true; // удалять или нет 
    
    for(unsigned int i=0; i<how_many;i++)
    {
    
        if(    bl[i].binding_type == CosNaming::nobject)
          {
//            cout <<"удаляем "<< omniURI::nameToString(bl[i].binding_name) << endl;
            ctx->unbind(bl[i].binding_name);
        }
        else if( bl[i].binding_type == CosNaming::ncontext)
        {

            if( recursive )
            {
                ulogrep << "ORepFactory: удаляем рекурсивно..." << endl;
                string rctx = fullName+"/"+omniURI::nameToString(bl[i].binding_name);
                ulogrep << rctx << endl;
                if ( !removeSection(rctx))
                {
                    ulogrep << "рекурсивно удалить не удалось" << endl;
                    rem = false;
                }
            }
            else
            {
                ulogrep << "ORepFactory: " << omniURI::nameToString(bl[i].binding_name) << " - контекст!!! ";
                ulogrep << "ORepFactory: пока не удаляем" << endl;
                rem = false;
            }
        }
    }


    // Удаляем контекст, если он уже пустой
    if( rem )
    {
        // Получаем имя контекста содержащего удаляемый
        string in_sec(ORepHelpers::getSectionName(fullName));
        //Получаем имя удаляемого контекста
        string name(ObjectIndex::getBaseName(fullName));

        try
        {
            int argc(uconf->getArgc());
            const char * const* argv(uconf->getArgv());
            CosNaming::Name_var ctxName = omniURI::stringToName(name.c_str());
            CosNaming::NamingContext_var in_ctx = ORepHelpers::getContext(in_sec, argc, argv, nsName);
            ctx->destroy();
            in_ctx->unbind(ctxName);
        }
        catch(const CosNaming::NamingContext::NotEmpty &ne)
        {
            ulogrep << "ORepFactory: контекст" << fullName << " не пустой " << endl;
            rem = false;
        }
        catch( ORepFailed )
        {
            ulogrep << "ORepFactory: не удаось получить ссылку на контекст " << in_sec << endl;
            rem=false;
        }
    }


    bi->destroy(); // ??
    return rem;
}

// -----------------------------------------------------------------------------------------------------------
/*!
 * \param newFName - полное имя новой секции
 * \param oldFName - полное имя удаляемрй секции
*/
bool ObjectRepositoryFactory::renameSection( const string& newFName, const string& oldFName )
{
    string newName(ObjectIndex::getBaseName(newFName));
    string oldName(ObjectIndex::getBaseName(oldFName));
    CosNaming::Name_var ctxNewName = omniURI::stringToName(newName.c_str());
    CosNaming::Name_var ctxOldName = omniURI::stringToName(oldName.c_str());

    string in_sec(ORepHelpers::getSectionName(newFName));

    try
    {
        int argc(uconf->getArgc());
        const char * const* argv(uconf->getArgv());
        CosNaming::NamingContext_var in_ctx = ORepHelpers::getContext(in_sec, argc, argv, nsName);    
        CosNaming::NamingContext_var ctx = ORepHelpers::getContext(oldFName, argc, argv, nsName);    

        // заменит контекст newFName если он существовал
        in_ctx->rebind_context(ctxNewName, ctx);
        in_ctx->unbind(ctxOldName);
    }
    catch( ORepFailed )
    {
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------------------------------------
