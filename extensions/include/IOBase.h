// -----------------------------------------------------------------------------
#ifndef IOBase_H_
#define IOBase_H_
// -----------------------------------------------------------------------------
#include <string>
#include "PassiveTimer.h"
#include "Trigger.h"
#include "Mutex.h"
#include "DigitalFilter.h"
#include "Calibration.h"
#include "IOController.h"
#include "SMInterface.h"
// -----------------------------------------------------------------------------
static const int DefaultSubdev     = -1;
static const int DefaultChannel = -1;
static const int NoSafety = -1;
// -----------------------------------------------------------------------------
        /*! Информация о входе/выходе */
        struct IOBase
        {
            IOBase():
                cdiagram(0),
                breaklim(0),
                value(0),
                craw(0),
                safety(0),
                defval(0),
                df(1),
                nofilter(false),
                f_median(false),
                f_ls(false),
                f_filter_iir(false),
                ignore(false),
                invert(false),
                noprecision(false),
                jar_state(false),
                ondelay_state(false),
                offdelay_state(false),
                t_ai(UniSetTypes::DefaultObjectId),
                front(false),
                front_type(ftUnknown),
                front_prev_state(false),
                front_state(false)
            {}


            bool check_channel_break( long val );     /*!< проверка обрыва провода */

            bool check_jar( bool val );            /*!< реализация фильтра против дребезга */
            bool check_on_delay( bool val );    /*!< реализация задержки на включение */
            bool check_off_delay( bool val );    /*!< реализация задержки на отключение */
            bool check_front( bool val );        /*!< реализация срабатывания по фронту сигнала */

            IOController_i::SensorInfo si;
            UniversalIO::IOType stype;            /*!< тип канала (DI,DO,AI,AO) */
            IOController_i::CalibrateInfo cal;     /*!< калибровочные параметры */
            Calibration* cdiagram;                /*!< специальная калибровочная диаграмма */

            long breaklim;     /*!< значение задающее порог определяющий обрыв (задаётся 'сырое' значение) */
            long value;        /*!< текущее значение */
            long craw;        /*!< текущее 'сырое' значение до калибровки */
            long cprev;        /*!< предыдущее значение после калибровки */
            long safety;    /*!< безопасное состояние при завершении процесса */
            long defval;    /*!< состояние по умолчанию (при запуске) */

            DigitalFilter df;    /*!< реализация программного фильтра */
            bool nofilter;        /*!< отключение фильтра */
            bool f_median;        /*!< признак использования медианного фильтра */
            bool f_ls;            /*!< признак использования адаптивного фильтра по методу наименьших квадратов */
            bool f_filter_iir;    /*!< признак использования рекурсивного фильтра */

            bool ignore;    /*!< игнорировать при опросе */
            bool invert;    /*!< инвертированная логика */
            bool noprecision;
            
            PassiveTimer ptJar;         /*!< таймер на дребезг */
            PassiveTimer ptOnDelay;     /*!< задержка на срабатывание */
            PassiveTimer ptOffDelay;     /*!< задержка на отпускание */
            
            bool jar_pause;
            Trigger trOnDelay;
            Trigger trOffDelay;
            Trigger trJar;
            
            bool jar_state;            /*!< значение для фильтра дребезга */
            bool ondelay_state;        /*!< значение для задержки включения */
            bool offdelay_state;    /*!< значение для задержки отключения */
            
            // Порог
            UniSetTypes::ObjectId t_ai; /*!< если данный датчик дискретный,
                                                и является пороговым, то в данном поле
                                                хранится идентификатор аналогового датчика
                                                с которым он связан */
            IONotifyController_i::ThresholdInfo ti;

            // Работа по фронтам сигнала
            enum FrontType
            {    
                ftUnknown,
                ft01,    // срабатывание на переход "0-->1"
                ft10  // срабатывание на переход "1-->0"
            };

            bool front; // флаг работы по фронту
            FrontType front_type;
            bool front_prev_state;
            bool front_state;
            
            IOController::IOStateList::iterator ioit;
            UniSetTypes::uniset_rwmutex val_lock;     /*!< блокировка на время "работы" со значением */
            
            friend std::ostream& operator<<(std::ostream& os, IOBase& inf );

            static void processingFasAI( IOBase* it, float new_val, SMInterface* shm, bool force );
            static void processingAsAI( IOBase* it, long new_val, SMInterface* shm, bool force );
            static void processingAsDI( IOBase* it, bool new_set, SMInterface* shm, bool force );
            static long processingAsAO( IOBase* it, SMInterface* shm, bool force );
            static float processingFasAO( IOBase* it, SMInterface* shm, bool force );
            static bool processingAsDO( IOBase* it, SMInterface* shm, bool force );
            static void processingThreshold( IOBase* it, SMInterface* shm, bool force );
            static bool initItem( IOBase* b, UniXML_iterator& it, SMInterface* shm,  
                                    DebugStream* dlog=0, std::string myname="",
                                    int def_filtersize=0, float def_filterT=0.0,
                                    float def_lsparam=0.2, float def_iir_coeff_prev=0.5,
                                    float def_iir_coeff_new=0.5 );
        };



// -----------------------------------------------------------------------------
#endif // IOBase_H_
// -----------------------------------------------------------------------------
