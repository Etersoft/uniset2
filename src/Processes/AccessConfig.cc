/*
 * Copyright (c) 2025 Pavel Vainerman.
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
#include <sstream>
#include "Exceptions.h"
#include "AccessConfig.h"
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
namespace uniset
{
    // --------------------------------------------------------------------------
    ACLMap AccessConfig::read( std::shared_ptr<Configuration>& conf,
                               const std::shared_ptr<UniXML>& uxml,
                               const std::string& name,
                               const std::string& section )
    {
        ACLMap out;

        if( !uxml || !conf )
            return out;

        auto root = uxml->findNode(uxml->getFirstNode(), section, name);

        if( !root )
            return out;

        UniXML::iterator it(root);

        if( !it.goChildren() )
            return out;

        ostringstream err;
        err << "(AccessConfig): file=" << uxml->getFileName() << " section=" << section << " confname=" << name;

        for( ; it; it++ )
        {
            auto aclName = it.getProp("name");

            if( aclName.empty() )
                throw SystemError(err.str() + " error: unknown ACL name");

            auto acl = std::make_shared<ACL>();
            acl->defaultPermissions = AccessUnknown;
            const std::string defStr = it.getProp("default");

            if( !defStr.empty() )
                acl->defaultPermissions = AccessMask::fromString(defStr);

            auto ia = it;

            if( !ia.goChildren() )
                throw SystemError(err.str() + " error: empty list for acl=" + aclName);

            for( ; ia; ia++ )
            {
                const std::string perm = ia.getProp("permission");

                if( perm.empty() )
                    throw SystemError(err.str() + " error: Unknown 'permission' in acl=" + aclName);

                ObjectId pid = DefaultObjectId;
                const std::string pname = ia.getProp("name");
                const string id(it.getProp("id"));

                if( !id.empty() )
                    pid = uni_atoi( id );
                else if( !pname.empty() )
                    pid = conf->getAnyID(pname);

                if( pid == DefaultObjectId )
                    throw SystemError(err.str().append(" error: Not found ID for '").append(pname).append( "' in acl=").append(aclName));

                acl->permissions[pid] = AccessMask::fromString(perm);
            }

            if( out.find(aclName) != out.end() )
                throw SystemError(err.str() + " duplicate acl=" + aclName);

            out[aclName] = acl;
        }

        return out;
    }
    // --------------------------------------------------------------------------
} // namespace uniset
// --------------------------------------------------------------------------
