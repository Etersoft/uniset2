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
// -----------------------------------------------------------------------------
#ifndef AccessConfig_H_
#define AccessConfig_H_
// -----------------------------------------------------------------------------
#include <list>
#include <memory>
#include <unordered_map>
#include <string>

#include "AccessMask.h"
#include "UniXML.h"
#include "Configuration.h"
// -----------------------------------------------------------------------------
namespace uniset
{
    using PermissionsMap = std::unordered_map<uniset::ObjectId, uniset::AccessMask>;
    struct ACL
    {
        AccessMask    defaultPermissions;
        PermissionsMap permissions;
    };

    using ACLMap = std::unordered_map< std::string, std::shared_ptr<ACL> >;
    using ACLPtr = std::shared_ptr<ACL>;

    struct ACLInfo
    {
        uniset::ObjectId sid = { uniset::DefaultObjectId };
        std::string name = { "" };
    };

    typedef std::unordered_map<uniset::ObjectId, ACLInfo> ACLInfoMap;

    /**
            <ACLConfig name="ACLConfig">
                <acl name="acl1" default="RW">
                    <access name="Process1" permission="RO"/>
                    <access name="Process2" permission="WRITE"/>
                </acl>
                <acl name="acl2" default="RW">
                    <access name="Process1" permission="RO"/>
                    <access name="Process3" permission="WRITE"/>
                </acl>
            </ACLConfig>
     */
    class AccessConfig
    {
        public:

            static ACLMap read( std::shared_ptr<Configuration>& conf,
                                const std::shared_ptr<UniXML>& _xml,
                                const std::string& name,
                                const std::string& section="ACLConfig" );

        private:
    };
    // -----------------------------------------------------------------------------
} // namespace uniset
// -----------------------------------------------------------------------------
#endif // AccessConfig_H_
// -----------------------------------------------------------------------------
