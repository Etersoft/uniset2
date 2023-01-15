/*
 * Copyright (c) 2020 Pavel Vainerman.
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
//----------------------------------------------------------------------------
#ifndef ClickHouseInterface_H_
#define ClickHouseInterface_H_
// ---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <memory>
#include <clickhouse-cpp/client.h>
#include <clickhouse-cpp/block.h>
#include <DBInterface.h>
// -------------------------------------------------------------------------
namespace uniset
{
    // ----------------------------------------------------------------------------
    class ClickHouseInterface:
        public DBNetInterface
    {
        public:

            ClickHouseInterface();
            ~ClickHouseInterface();

            virtual bool nconnect( const std::string& host, const std::string& user,
                                   const std::string& pswd, const std::string& dbname,
                                   unsigned int port = 9000) override;
            virtual bool close() override;
            virtual bool isConnection() const override;
            virtual bool ping() const override;

            virtual const std::string lastQuery() override;

            // Unsupport types: Array,Nullable,Tuple, idx
            virtual DBResult query( const std::string& q ) override;

            // supported all types
            const std::vector<clickhouse::Block> bquery( const std::string& q );

            bool execute( const std::string& q );
            bool insert( const std::string& tblname, const clickhouse::Block& data );

            virtual const std::string error() override;

            bool reconnect(const std::string& host, const std::string& user,
                           const std::string& pswd, const std::string& dbname,
                           unsigned int port = 9000);

            void setOptions( int sendRetries, bool pingBeforeQuery );

            // unsupported
            virtual bool insert( const std::string& q ) override
            {
                return false;
            }
            virtual double insert_id() override
            {
                return -1;
            }

        protected:

        private:
            DBResult makeResult( const clickhouse::Block& res );
            void appendResult( DBResult& ret, const clickhouse::Block& block );

            std::unique_ptr<clickhouse::Client> db;
            std::string lastQ;
            std::string lastE;

            int sendRetries = { 2 };
            bool pingBeforeQuery = { true };
    };
    // ----------------------------------------------------------------------------------
} // end of namespace uniset
// ----------------------------------------------------------------------------
#endif
// ----------------------------------------------------------------------------------
