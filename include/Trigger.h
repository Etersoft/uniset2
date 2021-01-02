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
 *  \brief Триггер, позволяющий красиво засекать изменения во флаге
 *  \author Vitaly Lipatov
*/
//--------------------------------------------------------------------------
#ifndef UNITRIGGER_H_
#define UNITRIGGER_H_
//--------------------------------------------------------------------------
namespace uniset
{
    // header only

    /*! Триггер, позволяющий красиво засекать изменения во флаге */
    class Trigger
    {
        public:
            Trigger(bool initial = false) noexcept
            {
                oldstate = initial;
            }

            /*! Срабатываем по верхнему фронту (при наступлении true) */
            bool hi(bool state) noexcept
            {
                if (oldstate != state)
                {
                    oldstate = state;

                    if (state)
                        return true;
                }

                return false;
            }
            /*! Срабатываем по нижнему фронту (при наступлении false) */
            bool low(bool state) noexcept
            {
                if (oldstate != state)
                {
                    oldstate = state;

                    if (!state)
                        return true;
                }

                return false;
            }
            /*! Срабатывает при любом изменении */
            bool change(bool state) noexcept
            {
                if (oldstate != state)
                {
                    oldstate = state;
                    return true;
                }

                return false;
            }

            inline bool get() const noexcept
            {
                return oldstate;
            }

        private:
            bool oldstate; /*!< предыдущее состояние */
    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
