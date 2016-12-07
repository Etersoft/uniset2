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
// --------------------------------------------------------------------------
/*! \file
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#include <omniORB4/CORBA.h>
#include <omniORB4/omniURI.h>
#include <string.h>
#include <sstream>
#include "ObjectRepository.h"
#include "ORepHelpers.h"
#include "UniSetObject_i.hh"
#include "Debug.h"
//#include "Configuration.h"
// --------------------------------------------------------------------------
using namespace omni;
using namespace uniset;
using namespace std;
// --------------------------------------------------------------------------

ObjectRepository::ObjectRepository( const std::shared_ptr<uniset::Configuration>& _conf ):
	nsName(_conf->getNSName()),
	uconf(_conf)
{
	init();
}


ObjectRepository::~ObjectRepository()
{
}

ObjectRepository::ObjectRepository():
	nsName("NameService")
{
	init();
}

bool ObjectRepository::init() const
{
	try
	{
		CORBA::ORB_var orb = uconf->getORB();
		{
			ostringstream s;
			s << uconf << "NameService";
			nsName = s.str();
		}
		localctx = ORepHelpers::getRootNamingContext(orb, nsName );

		if( CORBA::is_nil(localctx) )
			localctx = 0;
	}
	catch(...)
	{
		localctx = 0;
		return false;
	}

	return true;
}
// --------------------------------------------------------------------------
/*!
 *  Пример:  registration("sens1", oRef, "Root/SensorSection");
 * \param name - имя регистрируемого объекта
 * \param oRef - ссылка на объект
 * \param section - имя секции в которую заносится регистрационная запись
 * \exception ORepFailed - генерируется если произошла ошибка при регистрации
 * \sa registration(const std::string& fullName, const CORBA::Object_ptr oRef)
*/
void ObjectRepository::registration(const string& name, const ObjectPtr oRef, const string& section, bool force) const
throw(ORepFailed, ObjectNameAlready, InvalidObjectName, NameNotFound)
{
	ostringstream err;

	ulogrep << "ObjectRepository(registration): регистрируем " << name << endl;

	if( CORBA::is_nil(oRef) )
	{
		err << "ObjectRepository(registrartion): oRef=nil! for '" << name << "'";
		throw ORepFailed(err.str());
	}

	if( name.empty() )
	{
		err << "ObjectRepository(registration): (InvalidObjectName): empty name?!";
		throw ( InvalidObjectName(err.str()) );
	}

	// Проверка корректности имени
	char bad = ORepHelpers::checkBadSymbols(name);

	if( bad != 0 )
	{
		err << "ObjectRepository(registration): (InvalidObjectName) " << name;
		err << " содержит недопустимый символ " << bad;
		throw ( InvalidObjectName(err.str()) );
	}

	CosNaming::Name_var oName = omniURI::stringToName(name.c_str());
	CosNaming::NamingContext_var ctx =  CosNaming::NamingContext::_nil();

	for( unsigned int i = 0; i < 2; i++ )
	{
		try
		{
			// Добавляем в репозиторий новую ссылку (заменяя если есть старую)
			CORBA::ORB_var orb = uconf->getORB();
			ctx = ORepHelpers::getContext(orb, section, nsName);

			ctx->bind(oName, oRef);
			return;
		}
		catch(const CosNaming::NamingContext::AlreadyBound& nf)
		{
			uwarn << "(registration): " << name << " уже зарегестрирован в " << section << "!!!" << endl;

			if( !force )
				throw ObjectNameAlready();

			// разрегистриуем, перед повтроной попыткой
			if( !CORBA::is_nil(ctx) )
				ctx->unbind(oName);

			continue;
		}
		catch( const ORepFailed )
		{
			string er("ObjectRepository(registrartion): (getContext) не смог зарегистрировать " + name);
			throw ORepFailed(er);
		}
		catch( const CosNaming::NamingContext::NotFound& )
		{
			throw NameNotFound();
		}
		catch( const CosNaming::NamingContext::InvalidName& nf )
		{
			err << "ObjectRepository(registration): (InvalidName) не смог зарегистрировать ссылку  " << name;;
		}
		catch( const CosNaming::NamingContext::CannotProceed& cp )
		{
			err << "ObjectRepository(registrartion): catch CannotProced " << name << " bad part=";
			err << omniURI::nameToString(cp.rest_of_name);
		}
		catch( const CORBA::SystemException& ex )
		{
			uwarn << "ObjectRepository(registrartion): поймали CORBA::SystemException: "
				  << ex.NP_minorString() << endl;

			err << "ObjectRepository(registrartion): поймали CORBA::SystemException: " << ex.NP_minorString();
		}
	}

	if( err.str().empty() )
		err << "(ObjectRepository(registrartion): unknown error..";

	throw ORepFailed(err.str());
}
// --------------------------------------------------------------------------

/*!
 *  Функция регистрирует объект с именем "fullName" в репозитории объектов и связывает это имя со сылкой "oRef".
 *  \note При этом надо иметь ввиду, что задается полное имя объекта.
 *    Пример: registration("Root/SensorSection/sens1", oRef);
 *    \param fullName - полное имя регистрируемого объекта (т.е. включающее в себя имя секции)
 *    \param oRef - ссылка на объект
 *  \exception ORepFailed - генерируется если произошла ошибка при регистрации
 *  \sa registration(const string name, const ObjectPtr oRef, const string section)
*/
void ObjectRepository::registration( const std::string& fullName, const uniset::ObjectPtr oRef, bool force ) const
throw(ORepFailed, ObjectNameAlready, InvalidObjectName, NameNotFound)
{
	//    string n(ORepHelpers::getShortName(fullName));
	string n( uconf->oind->getBaseName(fullName) );
	string s(ORepHelpers::getSectionName(fullName));
	registration(n, oRef, s, force);
}
// --------------------------------------------------------------------------

/*!
 *    \param name - имя регистрируемого объекта (т.е. включающее в себя имя секции)
 *    \param section - имя секции в которой зарегистрирован объект
 *  \exception ORepFailed - генерируется если произошла ошибка при удалении
 *     \warning Нет проверки корректности удаляемого имени. т.е.
 *    проверки на, то не является ли имя ссылкой на объект или контекст
 *    т.к. для удаления ссылки на контекст нужен алгоритм посложнее...
*/
void ObjectRepository::unregistration(const string& name, const string& section) const
throw(ORepFailed, NameNotFound)
{
	ostringstream err;
	CosNaming::Name_var oName = omniURI::stringToName(name.c_str());
	CosNaming::NamingContext_var ctx;
	CORBA::ORB_var orb = uconf->getORB();
	ctx = ORepHelpers::getContext(orb, section, nsName);

	try
	{
		// Удаляем запись об объекте
		ctx->unbind(oName);
		return;
	}
	catch(const CosNaming::NamingContext::NotFound& nf)
	{
		err << "ObjectRepository(unregistrartion): не найден объект ->" << name;
	}
	catch(const CosNaming::NamingContext::InvalidName& in)
	{
		err << "ObjectRepository(unregistrartion): не корректное имя объекта -> " << name;
	}
	catch(const CosNaming::NamingContext::CannotProceed& cp)
	{
		err << "ObjectRepository(unregistrartion): catch CannotProced " << name << " bad part=";
		err << omniURI::nameToString(cp.rest_of_name);
	}

	if (err.str().empty())
		err << "ObjectRepository(unregistrartion): не смог удалить " << name;

	throw ORepFailed(err.str());
}
// --------------------------------------------------------------------------
/*!
 *    \param fullName - полное имя регистрируемого объекта (т.е. включающее в себя имя секции)
 *  \exception ORepFailed - генерируется если произошла ошибка при удалении
 *     \sa unregistration(const string name, const string section)
*/
void ObjectRepository::unregistration(const string& fullName) const
throw(ORepFailed, NameNotFound)
{
	//    string n(ORepHelpers::getShortName(fullName));
	string n(uconf->oind->getBaseName(fullName));
	string s(ORepHelpers::getSectionName(fullName));
	unregistration(n, s);
}
// --------------------------------------------------------------------------

ObjectPtr ObjectRepository::resolve( const string& name, const string& NSName ) const
throw(ORepFailed,  NameNotFound)
{
	ostringstream err;

	try
	{
		if( !localctx && !init() )
			throw ORepFailed("ObjectRepository(resolve): не смог получить ссылку на NameServices");

		CORBA::Object_var oRef;
		CosNaming::Name_var nc = omniURI::stringToName(name.c_str());
		oRef = localctx->resolve(nc);

		if ( !CORBA::is_nil(oRef) )
			return oRef._retn();

		err << "ObjectRepository(resolve): не смог получить ссылку на объект " << name.c_str();
	}
	catch(const CosNaming::NamingContext::NotFound& nf)
	{
		err << "ObjectRepository(resolve): NameNotFound name= " << name;
	}
	catch(const CosNaming::NamingContext::InvalidName& nf)
	{
		err << "ObjectRepository(resolve): не смог получить ссылку на контекст(InvalidName) ";
	}
	catch(const CosNaming::NamingContext::CannotProceed& cp)
	{
		err << "ObjectRepository(resolve): catch CannotProced " << name << " bad part=";
		err << omniURI::nameToString(cp.rest_of_name);
	}
	catch( const CORBA::SystemException& ex )
	{
		err << "ObjectRepository(resolve): catch SystemException: " << ex.NP_minorString()
			<< " для " << name;
	}
	catch(...)
	{
		err << "ObjectRepository(resolve): catch ... для " << name;
	}

	if( err.str().empty() )
		err << "ObjectRepository(resolve): unknown error for '" << name << "'";

	throw ORepFailed(err.str());
}

// --------------------------------------------------------------------------

/*!
 * \param ls - указатель на список который надо заполнить
 * \param how_many - максимальное количество заносимых элементов
 * \param section - полное имя секции начиная с Root.
 * \return Функция возвращает true, если в список были внесены не все элементы. Т.е. действительное
 * количество объектов в этой секции превышает заданное how_many.
 * \exception ORepFailed - генерируется если произошла при получении доступа к секции
*/
bool ObjectRepository::list(const string& section, ListObjectName* ls, size_t how_many)throw(ORepFailed)
{
	return list(section, ls, how_many, ObjectRef);
}

// --------------------------------------------------------------------------
/*!
 * \param ls - указатель на список который надо заполнить
 * \param how_many - максимальное количество заносимых элементов
 * \param in_section - полное имя секции начиная с Root.
 * \return Функция возвращает true, если в список были внесены не все элементы. Т.е. действительное
 * количество объектов в этой секции превышает заданное how_many.
 * \exception ORepFailed - генерируется если произошла при получении доступа к секции
*/
bool ObjectRepository::listSections(const string& in_section, ListObjectName* ls, size_t how_many)throw(ORepFailed)
{
	return list(in_section, ls, how_many, Section);
}

// --------------------------------------------------------------------------
/*!
 * \param ls - указатель на список который надо заполнить
 * \param how_many - максимальное количество заносимых элементов
 * \param in_section - полное имя секции начиная с Root.
 * \param type - тип вынимаемых(заносимых в список) объектов.
 * \return Функция возвращает true, если в список были внесены не все элементы. Т.е. действительное
 * количество объектов в этой секции превышает заданное how_many.
 * \exception ORepFailed - генерируется если произошла при получении доступа к секции
*/
bool ObjectRepository::list(const string& section, ListObjectName* ls, unsigned int how_many, ObjectType type)
{
	// Возвращает false если вынут не весь список...
	CosNaming::NamingContext_var ctx;

	try
	{
		CORBA::ORB_var orb = uconf->getORB();
		ctx = ORepHelpers::getContext(orb, section, nsName);
	}
	catch( const ORepFailed )
	{
		uwarn << "ORepository(list): не смог получить ссылку на " << section << endl;
		throw;
	}

	if( CORBA::is_nil(ctx) )
	{
		uwarn << "ORepository(list): не смог получить ссылку на " << section << endl;
		throw ORepFailed();
	}

	CosNaming::BindingList_var bl;
	CosNaming::BindingIterator_var bi;
	ctx->list(how_many, bl, bi);

	// хитрая проверка на null приобращении к bl
	// coverity говорит потенциально это возможно
	// т.к. там возвращается указатель, который по умолчанию null
	if( !bl.operator->() )
		return false;

	bool res = true;

	if(how_many >= bl->length())
		how_many = bl->length();
	else
	{
		if ( bi != NULL )
			res = false;
	}

	//    cout << "получили список "<< section << " размером " << bl->length()<< endl;

	for( unsigned int i = 0; i < how_many; i++)
	{
		switch( type )
		{
			case ObjectRef:
			{
				if(bl[i].binding_type == CosNaming::nobject)
					ls->emplace_front(omniURI::nameToString(bl[i].binding_name));

				break;
			}

			case Section:
			{
				if( bl[i].binding_type == CosNaming::ncontext)
					ls->emplace_front(omniURI::nameToString(bl[i].binding_name));

				break;
			}
		}
	}

	return res;
}

// --------------------------------------------------------------------------
bool ObjectRepository::isExist( const string& fullName ) const
{
	try
	{
		CORBA::Object_var oRef = resolve(fullName, nsName);
		return isExist(oRef);
	}
	catch(...) {}

	return false;
}

// --------------------------------------------------------------------------

bool ObjectRepository::isExist( const ObjectPtr& oref ) const
{
	try
	{
		UniSetObject_i_var o = UniSetObject_i::_narrow(oref);
		return o->exist();
	}
	catch( const CORBA::TRANSIENT) {}
	catch( const CORBA::SystemException&) {}
	catch( const CORBA::Exception&) {}
	catch( const omniORB::fatalException& fe )
	{
		uwarn << "ObjectRepository(isExist): " << "поймали omniORB::fatalException:" << endl;
		uwarn << "  file: " << fe.file() << endl;
		uwarn << "  line: " << fe.line() << endl;
		uwarn << "  mesg: " << fe.errmsg() << endl;
	}
	catch(...) {}

	return false;
}

// --------------------------------------------------------------------------
/*!
 * \param name - имя создаваемой секции
 * \param in_section - полное имя секции внутри которой создается новая
 * \param section - полное имя секции начиная с Root.
 * \exception ORepFailed - генерируется если произошла при получении доступа к секции
*/
bool ObjectRepository::createSection(const string& name, const string& in_section)throw(ORepFailed, InvalidObjectName)
{
	char bad = ORepHelpers::checkBadSymbols(name);

	if (bad != 0)
	{
		ostringstream err;
		err << "ObjectRepository(registration): (InvalidObjectName) " << name;
		err << " содержит недопустимый символ " << bad;
		throw ( InvalidObjectName(err.str().c_str()) );
	}

	ulogrep << "CreateSection: name = " << name << "  in section = " << in_section << endl;

	if( in_section.empty() )
	{
		ulogrep << "CreateSection: in_section..empty" << endl;
		return createRootSection(name);
	}

	int argc(uconf->getArgc());
	const char* const* argv(uconf->getArgv());
	CosNaming::NamingContext_var ctx = ORepHelpers::getContext(in_section, argc, argv, uconf->getNSName() );
	return createContext( name, ctx.in() );
}
// -------------------------------------------------------------------------------------------------------
/*!
 * \param fullName - полное имя создаваемой секции
 * \exception ORepFailed - генерируется если произошла при получении доступа к секции
*/
bool ObjectRepository::createSectionF( const string& fullName )throw(ORepFailed, InvalidObjectName)
{
	string name(ObjectIndex::getBaseName(fullName));
	string sec(ORepHelpers::getSectionName(fullName));

	ulogrep << name << endl;
	ulogrep << sec << endl;

	if ( sec.empty() )
	{
		ulogrep << "SectionName is empty!!!" << endl;
		ulogrep << "Добавляем в " << uconf->getRootSection() << endl;
		return createSection(name, uconf->getRootSection());
	}
	else
		return createSection(name, sec);
}

// ---------------------------------------------------------------------------------------------------------------
bool ObjectRepository::createRootSection( const string& name )
{
	CORBA::ORB_var orb = uconf->getORB();
	CosNaming::NamingContext_var ctx = ORepHelpers::getRootNamingContext(orb, uconf->getNSName());
	return createContext(name, ctx);
}
// -----------------------------------------------------------------------------------------------------------
bool ObjectRepository::createContext( const string& cname, CosNaming::NamingContext_ptr ctx )
{
	CosNaming::Name_var nc = omniURI::stringToName(cname.c_str());

	try
	{
		ulogrep << "ORepFactory(createContext): создаю новый контекст " << cname << endl;
		ctx->bind_new_context(nc);
		ulogrep << "ORepFactory(createContext): создал. " << endl;
		return true;
	}
	catch(const CosNaming::NamingContext::AlreadyBound& ab)
	{
		//        ctx->resolve(nc);
		ulogrep << "ORepFactory(createContext): context " << cname << " уже есть" << endl;
		return true;
	}
	catch( const CosNaming::NamingContext::NotFound )
	{
		ulogrep << "ORepFactory(createContext): NotFound " << cname << endl;
		throw NameNotFound();
	}
	catch( const CosNaming::NamingContext::InvalidName& nf )
	{
		uwarn << "ORepFactory(createContext): (InvalidName)  " << cname;
	}
	catch( const CosNaming::NamingContext::CannotProceed& cp )
	{
		uwarn << "ORepFactory(createContext): catch CannotProced "
			  << cname << " bad part="
			  << omniURI::nameToString(cp.rest_of_name);
		throw NameNotFound();
	}
	catch( const CORBA::SystemException& ex )
	{
		ucrit << "ORepFactory(createContext): CORBA::SystemException: "
			  << ex.NP_minorString() << endl;
	}
	catch( const CORBA::Exception& )
	{
		ucrit << "поймали CORBA::Exception." << endl;
	}
	catch( const omniORB::fatalException& fe )
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
void ObjectRepository::printSection( const string& fullName )
{
	ListObjectName ls;

	try
	{
		list(fullName.c_str(), &ls);

		if( ls.empty() )
			cout << fullName << " пуст!!!" << endl;
	}
	catch( ORepFailed )
	{
		cout << "printSection: cath exceptions ORepFailed..." << endl;
		return ;
	}

	cout << fullName << "(" << ls.size() << "):" << endl;

	for( auto v : ls )
		cout << v << endl;
}

// -----------------------------------------------------------------------------------------------------------
/*!
 * \param fullName - имя удаляемой секции
 * \param recursive - удлаять рекурсивно все секции или возвращать не удалять и ошибку ( временно )
 * \warning Функция вынимает только первые 1000 объектов, остальные игнорируются...
*/
bool ObjectRepository::removeSection(const string& fullName, bool recursive)
{
	//    string name = getName(fullName.c_str(),'/');
	//  string sec = getSectionName(fullName.c_str(),'/');
	//    CosNaming::NamingContext_var ctx = getContext(sec, argc, argv);
	unsigned int how_many = 1000;
	CosNaming::NamingContext_var ctx;

	try
	{
		int argc(uconf->getArgc());
		const char* const* argv(uconf->getArgv());
		ctx = ORepHelpers::getContext(fullName, argc, argv, nsName);
	}
	catch( ORepFailed )
	{
		return false;
	}

	CosNaming::BindingList_var bl;
	CosNaming::BindingIterator_var bi;

	ctx->list(how_many, bl, bi);

	// хитрая проверка на null приобращении к bl
	// coverity говорит потенциально это возможно
	// т.к. там возвращается указатель, который по умолчанию null
	if( !bl.operator->() )
		return false;

	if( how_many > bl->length() )
		how_many = bl->length();

	bool rem = true; // удалять или нет

	for(unsigned int i = 0; i < how_many; i++)
	{

		if( bl[i].binding_type == CosNaming::nobject)
		{
			//            cout <<"удаляем "<< omniURI::nameToString(bl[i].binding_name) << endl;
			ctx->unbind(bl[i].binding_name);
		}
		else if( bl[i].binding_type == CosNaming::ncontext)
		{

			if( recursive )
			{
				ulogrep << "ORepFactory: удаляем рекурсивно..." << endl;
				string rctx = fullName + "/" + omniURI::nameToString(bl[i].binding_name);
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
			const char* const* argv(uconf->getArgv());
			CosNaming::Name_var ctxName = omniURI::stringToName(name.c_str());
			CosNaming::NamingContext_var in_ctx = ORepHelpers::getContext(in_sec, argc, argv, nsName);
			ctx->destroy();
			in_ctx->unbind(ctxName);
		}
		catch(const CosNaming::NamingContext::NotEmpty& ne)
		{
			ulogrep << "ORepFactory: контекст" << fullName << " не пустой " << endl;
			rem = false;
		}
		catch( ORepFailed )
		{
			ulogrep << "ORepFactory: не удаось получить ссылку на контекст " << in_sec << endl;
			rem = false;
		}
	}


	if( !CORBA::is_nil(bi) )
		bi->destroy(); // ??

	return rem;
}

// -----------------------------------------------------------------------------------------------------------
/*!
 * \param newFName - полное имя новой секции
 * \param oldFName - полное имя удаляемрй секции
*/
bool ObjectRepository::renameSection( const string& newFName, const string& oldFName )
{
	string newName(ObjectIndex::getBaseName(newFName));
	string oldName(ObjectIndex::getBaseName(oldFName));
	CosNaming::Name_var ctxNewName = omniURI::stringToName(newName.c_str());
	CosNaming::Name_var ctxOldName = omniURI::stringToName(oldName.c_str());

	string in_sec(ORepHelpers::getSectionName(newFName));

	try
	{
		int argc(uconf->getArgc());
		const char* const* argv(uconf->getArgv());
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
