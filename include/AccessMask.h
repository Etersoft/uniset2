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
#ifndef AccessMask_H_
#define AccessMask_H_
// --------------------------------------------------------------------------
#include <cstdint>
#include <string>
#include <algorithm>
#include <initializer_list>
#include <ostream>
// --------------------------------------------------------------------------
namespace uniset
{
    class AccessMask
    {
        public:
            enum Flag : uint8_t
            {
                UNKNOWN = 0,
                NONE  = 1 << 0,
                READ  = 1 << 1,
                WRITE = 1 << 2
            };

            constexpr AccessMask() noexcept = default;
            constexpr AccessMask(uint8_t m) noexcept : mask_(m) {}
            constexpr AccessMask(Flag f) noexcept : mask_(f) {}
            constexpr AccessMask(std::initializer_list<Flag> flags) noexcept
            {
                for (auto f : flags) mask_ |= f;
            }

            constexpr bool canRead() const noexcept
            {
                return mask_ & READ;
            }

            constexpr bool canWrite() const noexcept
            {
                return mask_ & WRITE;
            }

            constexpr bool empty() const noexcept
            {
                return mask_ == 0;
            }

            constexpr bool has(const AccessMask& other) const noexcept
            {
                return (mask_ & other.mask_) == other.mask_;
            }

            constexpr void add(const AccessMask& other) noexcept
            {
                mask_ |= other.mask_;
            }

            constexpr void remove(const AccessMask& other) noexcept
            {
                mask_ &= ~other.mask_;
            }

            constexpr void clear() noexcept
            {
                mask_ = 0;
            }

            constexpr uint8_t raw() const noexcept
            {
                return mask_;
            }

            std::string toString() const
            {
                switch (mask_)
                {
                    case NONE:
                        return "None";

                    case READ:
                        return "RO";

                    case READ | WRITE:
                        return "RW";

                    case UNKNOWN:
                        return "Unknown";

                    default:
                        return "Unknown(" + std::to_string(mask_) + ")";
                }
            }

            // сравнения и операции
            constexpr bool operator==(const AccessMask& o) const noexcept
            {
                return mask_ == o.mask_;
            }
            constexpr bool operator!=(const AccessMask& o) const noexcept
            {
                return mask_ != o.mask_;
            }

            constexpr AccessMask operator|(const AccessMask& o) const noexcept
            {
                if( mask_ == NONE )
                    return *this;
                return mask_ | o.mask_;
            }
            constexpr AccessMask operator&(const AccessMask& o) const noexcept
            {
                if( mask_ == NONE )
                    return *this;

                return mask_ & o.mask_;
            }

            constexpr AccessMask& operator|=(const AccessMask& o) noexcept
            {
                if( mask_ == NONE )
                    return *this;

                mask_ |= o.mask_;
                return *this;
            }
            constexpr AccessMask& operator&=(const AccessMask& o) noexcept
            {
                if( mask_ == NONE )
                    return *this;
                mask_ &= o.mask_;
                return *this;
            }

            static std::string toLower( const std::string& s )
            {
                std::string res = s;
                std::transform(res.begin(), res.end(), res.begin(),
                               [](unsigned char c)
                {
                    return std::tolower(c);
                });
                return res;
            }

            static AccessMask fromString( const std::string& m )
            {
                if( m.empty() )
                    return AccessMask::UNKNOWN;

                auto s = toLower(m);

                if( s == "rd" || s == "read" || s == "ro" || s == "r" )
                    return AccessMask::READ;

                if( s == "wr" || s == "write" || s == "w" )
                    return AccessMask::WRITE;

                if( s == "rw"  )
                    return AccessMask::READ | AccessMask::WRITE;

                if( s == "none" )
                    return AccessMask::NONE;

                return AccessMask::UNKNOWN;
            }

        private:
            uint8_t mask_ { UNKNOWN };
    };

    inline std::ostream& operator<<(std::ostream& os, const AccessMask& a)
    {
        return os << a.toString();
    }

    constexpr AccessMask AccessUnknown = AccessMask(AccessMask::UNKNOWN);
    constexpr AccessMask AccessNone = AccessMask(AccessMask::NONE);
    constexpr AccessMask AccessRO   = AccessMask(AccessMask::READ);
    constexpr AccessMask AccessRW   = AccessMask({AccessMask::READ, AccessMask::WRITE});
    // --------------------------------------------------------------------------
}    // end of uniset namespace
// --------------------------------------------------------------------------
#endif // AccessMask_H_
