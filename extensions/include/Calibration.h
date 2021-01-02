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
#ifndef Calibration_H_
#define Calibration_H_
// -----------------------------------------------------------------------------
#include <cmath>
#include <string>
#include <vector>
#include <deque>
#include <ostream>
//--------------------------------------------------------------------------
namespace uniset
{
    // -----------------------------------------------------------------------------
    /*!
        Класс позволяющий загружать калибровочную
    характеристику из конфигурационного (xml)файла и получать по ней точки.
    \code
    C, калиброванное значение
      ^
      |
      |
      |
      |
       -------->
               R(raw value), сырое значение
    \endcode

    Сами диаграммы представляют из себя следующую секцию в xml
    x - сырое значение (по которому ведётся поиск)
    y - калиброванное значение
    \code
    <Calibrations name="Calibrations">
        <diagram name="testcal">
            <point x="-200" y="-60"/>
            <point x="-100" y="-60"/>
            <point x="-50" y="-20"/>
            <point x="0" y="0"/>
            <point x="50" y="20"/>
            <point x="100" y="60"/>
            <point x="200" y="60"/>
        </diagram>
        <diagram name="NNN">
        ...
        </diagram>
        <diagram name="ZZZ">
        ...
        </diagram>
        ...
    </Calibrations>
    \endcode

    Диаграмма позволяет задать множество точек. На отрезках между точками используется линейная аппроксимация.

    Т.к. часто большую часть времени (во многих задачах) аналоговое значение, меняется в небольших пределах,
    то добавлен кэш ( rawValue --> calValue ) по умолчанию на 5 значений. Размер кэша можно задать
    (поменять или отключить) при помощи Calibration::setCacheSize().
    \note Слишком большим задавать кэш не рекомендуется, т.к. тогда поиск по кэшу будет сопоставим с поиском по диаграмме.

      Помимо этого, с учётом того, что каждое попадание в кэш обновляет счётчик обращений к значению, необходимо
    пересортировывать весь кэш (чтобы наиболее часто используемые были в начале). Чтобы не делать эту операцию каждый
    раз, сделан счётчик циклов. Т.е. через какое количество обращений к кэшу, производить принудительную пересортировку.
    Значение по умолчанию - 5(размер кэша). Задать можно при помощи Calibration::setCacheResortCycle()
    */
    class Calibration
    {
        public:
            Calibration();
            Calibration( const std::string& name, const std::string& confile = "calibration.xml", size_t reserv = 50 );
            Calibration( xmlNode* node, size_t reserv = 50 );
            ~Calibration();

            /*! Тип для хранения значения */
            typedef float TypeOfValue;

            /*! выход за границы диапазона (TypeOfValue) */
            static const TypeOfValue ValueOutOfRange;

            /*! выход за границы диапазона */
            static const long outOfRange;

            /*!
                Получение калиброванного значения
                \param raw - сырое значение
                \param crop_raw - обрезать переданное значение по крайним точкам
                \return Возвращает калиброванное или outOfRange
            */
            long getValue( const long raw, bool crop_raw = false );

            /*! Возвращает минимальное значение 'x' встретившееся в диаграмме */
            inline long getMinValue() const noexcept
            {
                return minVal;
            }
            /*! Возвращает максимальное значение 'x' встретившееся в диаграмме */
            inline long getMaxValue() const noexcept
            {
                return maxVal;
            }

            /*! Возвращает крайнее левое значение 'x' встретившееся в диаграмме (ПОСЛЕ СОРТИРОВКИ ПО ВОЗРАСТАНИЮ 'x'!) */
            inline long getLeftValue() const noexcept
            {
                return leftVal;
            }
            /*! Возвращает крайнее правое значение 'x' встретившееся в диаграмме (ПОСЛЕ СОРТИРОВКИ ПО ВОЗРАСТАНИЮ 'x'!) */
            inline long getRightValue() const noexcept
            {
                return rightVal;
            }

            /*!
                Получение сырого значения по калиброванному
                \param range=true вернуть крайнее значение в диаграмме
                    если cal < leftVal или cal > rightVal (т.е. выходит за диапазон)

                Если range=false, то может быть возвращено значение outOfRange.
            */
            long getRawValue( const long cal, bool range = false ) const;

            /*! Возвращает минимальное значение 'y' встретившееся в диаграмме */
            inline long getMinRaw() const noexcept
            {
                return minRaw;
            }
            /*! Возвращает максимальное значение 'y' встретившееся в диаграмме */
            inline long getMaxRaw() const noexcept
            {
                return maxRaw;
            }

            /*! Возвращает крайнее левое значение 'y' встретившееся в диаграмме (ПОСЛЕ СОРТИРОВКИ ПО ВОЗРАСТАНИЮ 'x'!) */
            inline long getLeftRaw() const noexcept
            {
                return leftRaw;
            }
            /*! Возвращает крайнее правое значение 'y' встретившееся в диаграмме (ПОСЛЕ СОРТИРОВКИ ПО ВОЗРАСТАНИЮ 'x'!) */
            inline long getRightRaw() const noexcept
            {
                return rightRaw;
            }

            /*! построение характеристики из конф. файла
                \param name - название характеристики в файле
                \param confile - файл содержащий данные
                \param node    - если node!=0, то используется этот узел...
            */
            void build( const std::string& name, const std::string& confile, xmlNode* node = 0  );

            /*! преобразование типа для хранения
                в тип для аналоговых датчиков
             */
            inline long tRound( const TypeOfValue& val ) const
            {
                return lround(val);
            }

            void setCacheSize( size_t sz );

            inline size_t getCacheSize() const
            {
                return cache.size();
            }

            void setCacheResortCycle( size_t n );
            inline size_t getCacheResotrCycle() const noexcept
            {
                return numCacheResort;
            }
            // ---------------------------------------------------------------

            friend std::ostream& operator<<(std::ostream& os, Calibration& c );
            friend std::ostream& operator<<(std::ostream& os, Calibration* c );

            // ---------------------------------------------------------------
            /*!    точка характеристики */
            struct Point
            {
                Point(): x(outOfRange), y(outOfRange) {}

                Point( TypeOfValue _x, TypeOfValue _y ):
                    x(_x), y(_y) {}

                TypeOfValue x;
                TypeOfValue y;

                inline bool operator < ( const Point& p ) const
                {
                    return ( x < p.x );
                }
            };

            /*! участок характеристики */
            class Part
            {
                public:
                    Part() noexcept;
                    Part( const Point& pleft, const Point& pright ) noexcept;
                    ~Part() {};

                    /*! находится ли точка на данном участке */
                    bool check( const Point& p ) const noexcept;

                    /*! находится ли точка на данном участке по X */
                    bool checkX( const TypeOfValue& x ) const noexcept;

                    /*!    находится ли точка на данном участке по Y */
                    bool checkY( const TypeOfValue& y ) const noexcept;

                    // функции могут вернуть OutOfRange
                    TypeOfValue getY( const TypeOfValue& x ) const noexcept;      /*!< получить значение Y */
                    TypeOfValue getX( const TypeOfValue& y ) const noexcept;   /*!< получить значение X */

                    TypeOfValue calcY( const TypeOfValue& x ) const noexcept;  /*!< рассчитать значение для x */
                    TypeOfValue calcX( const TypeOfValue& y ) const noexcept;  /*!< рассчитать значение для y */

                    inline bool operator < ( const Part& p ) const noexcept
                    {
                        return (p_right < p.p_right);
                    }

                    inline Point leftPoint() const noexcept
                    {
                        return p_left;
                    }
                    inline Point rightPoint() const noexcept
                    {
                        return p_right;
                    }
                    inline TypeOfValue getK() const noexcept
                    {
                        return k;    /*!< получить коэффициент наклона */
                    }
                    inline TypeOfValue left_x() const noexcept
                    {
                        return p_left.x;
                    }
                    inline TypeOfValue left_y() const noexcept
                    {
                        return p_left.y;
                    }
                    inline TypeOfValue right_x() const noexcept
                    {
                        return p_right.x;
                    }
                    inline TypeOfValue right_y() const noexcept
                    {
                        return p_right.y;
                    }

                protected:
                    Point p_left;  /*!< левый предел участка */
                    Point p_right; /*!< правый предел участка */
                    TypeOfValue k; /*!< коэффициент наклона */
            };

            // список надо отсортировать по x!
            typedef std::vector<Part> PartsVec;

            inline std::string getName()
            {
                return myname;
            }

        protected:

            long minRaw, maxRaw, minVal, maxVal, rightVal, leftVal, rightRaw, leftRaw;

            void insertToCache( const long raw, const long val );

        private:
            PartsVec pvec;
            std::string myname;

            // Cache
            size_t szCache;
            struct CacheInfo
            {
                CacheInfo() noexcept: val(0), raw(outOfRange), cnt(0) {}
                CacheInfo( const long r, const long v ) noexcept: val(v), raw(r), cnt(0) {}

                long val;
                long raw;
                size_t cnt; // счётчик обращений

                // сортируем в порядке убывания(!) обращений
                // т.е. наиболее часто используемые (впереди)
                inline bool operator<( const CacheInfo& r ) const noexcept
                {
                    if( r.raw == outOfRange )
                        return true;

                    // неинициализированные записи, сдвигаем в конец.
                    if( raw == outOfRange )
                        return false;

                    return cnt > r.cnt;
                }
            };

            typedef std::deque<CacheInfo> ValueCache;
            ValueCache cache;
            size_t numCacheResort; // количество обращений, при которых происходит перестроение (сортировка) кэша..
            size_t numCallToCache; // текущий счётчик обращений к кэшу
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset
// -----------------------------------------------------------------------------
#endif // Calibration_H_
// -----------------------------------------------------------------------------
