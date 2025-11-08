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
// -----------------------------------------------------------------------------
#ifndef FakeIOControl_H_
#define FakeIOControl_H_
// -----------------------------------------------------------------------------
#include <memory>
#include "IOControl.h"
// -------------------------------------------------------------------------
namespace uniset
{
    /*! Специальный "управляемый тестами" интерфейс карты в/в. */
    class FakeComediInterface final:
        public ComediInterface
    {
        public:
            FakeComediInterface();
            virtual ~FakeComediInterface();

            static const size_t maxChannelNum = 32;

            // Управление тестированием
            // --------------------------------------------
            // для простоты массивы специально объявлены в public
            // warning: надо только иметь ввиду, что доступ к ним будет из разных потоков
            // из теста и из потока опроса карт в/в (IOControl)
            // в данном случае это не страшно..
            // --------------------------------------------
            std::vector<int> chInputs; // массив значений 'входов' (для проверки чтения)
            mutable std::vector<int> chOutputs; // массив значений 'выходов' (для проверки функций вывода)


            // --------------------------------------------
            // при тестировании параметры range,aref,subdev игнорируются!

            virtual int getAnalogChannel( int subdev, int channel, int range = 0, int aref = AREF_GROUND, int adelay = 10 * 1000 ) const override;

            virtual void setAnalogChannel( int subdev, int channel, int data, int range = 0, int aref = AREF_GROUND ) const override;

            virtual bool getDigitalChannel( int subdev, int channel ) const override;

            virtual void setDigitalChannel( int subdev, int channel, bool bit ) const override;

            virtual void configureSubdev( int subdev, SubdevType type ) const override;

            virtual void configureChannel( int subdev, int channel, ChannelType type, int range = 0, int aref = 0 ) const override;
    };
    // --------------------------------------------------------------------------
    /*! Специальный IOControl для тестирования подменяющий все карты в/в на FakeComediInterface */
    class FakeIOControl:
        public IOControl
    {
        public:
            FakeIOControl( uniset::ObjectId id, xmlNode* cnode,
                           uniset::ObjectId icID, const std::shared_ptr<SharedMemory>& shm = nullptr, int numcards = 2, const std::string& prefix = "io" );
            virtual ~FakeIOControl();

            /*! глобальная функция для инициализации объекта */
            static std::shared_ptr<FakeIOControl> init_iocontrol( int argc, const char* const* argv,
                    uniset::ObjectId icID, const std::shared_ptr<SharedMemory>& ic = nullptr,
                    const std::string& prefix = "io" );


            // намеренно делаем public для доступа в тестах
            FakeComediInterface* fcard = nullptr;

        protected:

        private:
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // FakeIOControl_H_
// -----------------------------------------------------------------------------
