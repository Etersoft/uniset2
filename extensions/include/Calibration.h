#ifndef Calibration_H_
#define Calibration_H_
// -----------------------------------------------------------------------------
#include <cmath>
#include <string>
#include <list>
#include <ostream>
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

*/
class Calibration
{
    public:
        Calibration();
        Calibration( const std::string& name, const std::string& confile="calibration.xml" );
        Calibration( xmlNode* node );
        ~Calibration();

        /*! выход за границы диапазона */
        static const int outOfRange=-1;

        /*!
            Получение калиброванного значения
            \param raw - сырое значение
            \param crop_raw - обрезать переданное значение по крайним точкам
            \return Возвращает калиброванное
        */
        long getValue( long raw, bool crop_raw=false );

        /*! Возвращает минимальное значение 'x' встретившееся в диаграмме */
        inline long getMinVal(){ return minVal; }
        /*! Возвращает максимальное значение 'x' втретившееся в диаграмме */
        inline long getMaxVal(){ return maxVal; }

        /*! Возвращает крайнее левое значение 'x' встретившееся в диаграмме (ПОСЛЕ СОРТИРОВКИ ПО ВОЗРАСТАНИЮ 'x'!) */
        inline long getLeftVal(){ return leftVal; }
        /*! Возвращает крайнее правое значение 'x' встретившееся в диаграмме (ПОСЛЕ СОРТИРОВКИ ПО ВОЗРАСТАНИЮ 'x'!) */
        inline long getRightVal(){ return rightVal; }

        /*!
            Получение сырого значения по калиброванному
            \param range=true вернуть крайнее значение в диаграмме
                если cal<leftVal или cal>rightVal (т.е. выходит за диапазон)

            Если range=false, то может быть возвращено значение outOfRange.
        */
        long getRawValue( long cal, bool range=false );

        /*! Возвращает минимальное значение 'y' встретившееся в диаграмме */
        inline long getMinRaw(){ return minRaw; }
        /*! Возвращает максимальное значение 'y' встретившееся в диаграмме */
        inline long getMaxRaw(){ return maxRaw; }

        /*! Возвращает крайнее левое значение 'y' встретившееся в диаграмме (ПОСЛЕ СОРТИРОВКИ ПО ВОЗРАСТАНИЮ 'x'!) */
        inline long getLeftRaw(){ return leftRaw; }
        /*! Возвращает крайнее правое значение 'y' встретившееся в диаграмме (ПОСЛЕ СОРТИРОВКИ ПО ВОЗРАСТАНИЮ 'x'!) */
        inline long getRightRaw(){ return rightRaw; }


        /*! построение характеристрики из конф. файла 
            \param name - название характеристики в файле
            \param confile - файл содержащий данные
            \param node    - если node!=0, то используется этот узел...
        */
        void build( const std::string& name, const std::string& confile, xmlNode* node=0  );


        /*! Тип для хранения текущего значения */
        typedef float TypeOfValue;

        /*! преобразование типа для хранения 
            в тип для аналоговых датчиков
         */
        inline long tRound( const TypeOfValue& val ) const
        {
            return lround(val);
        }

        friend std::ostream& operator<<(std::ostream& os, Calibration& c );
        friend std::ostream& operator<<(std::ostream& os, Calibration* c );

        /*!    точка характеристики */
        struct Point
        {
            Point( TypeOfValue _x, TypeOfValue _y ):
                x(_x),y(_y){}

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
                Part( const Point& pleft, const Point& pright );
                ~Part(){};

                /*!    находится ли точка на данном участке */
                bool check( const Point& p ) const;

                /*!    находится ли точка на данном участке по X */
                bool checkX( const TypeOfValue& x ) const;

                /*!    находится ли точка на данном участке по Y */
                bool checkY( const TypeOfValue& y ) const;

                // функции могут вернуть OutOfRange
                TypeOfValue getY( const TypeOfValue& x ) const;         /*!< получить значение Y */
                TypeOfValue getX( const TypeOfValue& y ) const;        /*!< получить значение X */

                TypeOfValue calcY( const TypeOfValue& x ) const;     /*!< расчитать значение для x */
                TypeOfValue calcX( const TypeOfValue& y ) const;     /*!< расчитать значение для y */

                inline bool operator < ( const Part& p ) const
                {
                    return (p_right < p.p_right);
                }

                inline Point leftPoint() const { return p_left; }
                inline Point rightPoint() const { return p_right; }
                inline TypeOfValue getK() const { return k; }     /*!< получить коэффициент наклона */
                inline TypeOfValue left_x() const { return p_left.x; }
                inline TypeOfValue left_y() const { return p_left.y; }
                inline TypeOfValue right_x() const { return p_right.x; }
                inline TypeOfValue right_y() const { return p_right.y; }

            protected:
                Point p_left;     /*!< левый предел участка */
                Point p_right;     /*!< правый предел участка */
                TypeOfValue k;     /*!< коэффициент наклона */
        };

    protected:

        // список надо отсортировать по x!
        typedef std::list<Part> PartsList;

        long minRaw, maxRaw, minVal, maxVal, rightVal, leftVal, rightRaw, leftRaw;

    private:
        PartsList plist;
        std::string myname;
};
// -----------------------------------------------------------------------------
#endif // Calibration_H_
// -----------------------------------------------------------------------------
