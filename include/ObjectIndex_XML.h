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
 * \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#ifndef ObjectIndex_XML_H_
#define ObjectIndex_XML_H_
// --------------------------------------------------------------------------
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include "ObjectIndex.h"
#include "UniXML.h"
// --------------------------------------------------------------------------
namespace uniset
{

    /*! реализация интерфейса ObjectIndex на основе xml-файла, с автоматическим назначением id объектам
     *
     * \todo Проверить функции этого класса на повторную входимость
    */
    class ObjectIndex_XML:
        public ObjectIndex
    {
        public:
            ObjectIndex_XML(const std::string& xmlfile, size_t minSize = 1000 );
            ObjectIndex_XML( const std::shared_ptr<UniXML>& xml, size_t minSize = 1000 );
            virtual ~ObjectIndex_XML();

            virtual const uniset::ObjectInfo* getObjectInfo( const ObjectId ) const noexcept override;
            virtual const uniset::ObjectInfo* getObjectInfo( const std::string& name ) const noexcept override;
            virtual ObjectId getIdByName( const std::string& name ) const noexcept override;
            virtual std::string getMapName( const ObjectId id ) const noexcept override;
            virtual std::string getTextName( const ObjectId id ) const noexcept override;

            virtual std::ostream& printMap(std::ostream& os) const noexcept override;
            friend std::ostream& operator<<(std::ostream& os, ObjectIndex_XML& oi );

        protected:
            void build( const std::shared_ptr<UniXML>& xml );
            size_t read_section(const std::shared_ptr<UniXML>& xml, const std::string& sec, size_t ind );
            size_t read_nodes( const std::shared_ptr<UniXML>& xml, const std::string& sec, size_t ind );

        private:
            typedef std::unordered_map<std::string, ObjectId> MapObjectKey;
            MapObjectKey mok; // для обратного писка
            std::vector<ObjectInfo> omap; // для прямого поиска
    };
    // -----------------------------------------------------------------------------------------
}    // end of namespace
// -----------------------------------------------------------------------------------------
#endif
