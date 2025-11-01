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
#ifndef JSProxy_H_
#define JSProxy_H_
// --------------------------------------------------------------------------
/*! \page pageJScript Поддержка скриптов на JS

    \tableofcontents

    \section sec_jsArch Архитектура JScript

    JScript предоставляет полноценную среду выполнения JavaScript с интеграцией
    в систему uniset.

    \subsection subsec_jsCore Базовые возможности
    - Загрузка и выполнение JS-скриптов
    - Интеграция с uniset
    - Поддержка модульной системы с поиском по указанным путям
    - готовые модули для типовых задач

    \subsection subsec_jsIO Работа с датчиками uniset
    JScript автоматически создает \b глобальные переменные для работы с датчиками:
    \code
    // Конфигурация в JS:
    uniset_inputs = [
        { name: "Sensor1" },
        { name: "Sensor2" }
    ];

    uniset_outputs = [
        { name: "Output1" },
        { name: "Output2" }
    ];

    // Использование в коде:
    function uniset_on_step() {
        // Чтение входных датчиков (автоматически создаются как глобальные переменные)
        let value = in_Sensor1;

        // Запись выходных датчиков
        out_Output1 = value + 10;
    }
    \endcode

    А так же создаются переменные с именем датчика, содержащие ID датчика
    \code
     console.log("Sensor1 id=", Sensor1)
    \endcode

    \subsection subsec_jsEvents Система событий
    JScript предоставляет набор callback-функций для реакции на системные события:

    \code
    // Вызывается при старте системы
    function uniset_on_start() {
        mylog.info("System started");
        // Инициализация таймеров и ресурсов
    }

    // Вызывается при остановке системы
    function uniset_on_stop() {
        mylog.info("System stopped");
        // Очистка ресурсов, остановка таймеров
    }

    // Вызывается на каждом шаге выполнения
    function uniset_on_step() {
        // Основная логика управления
        out_Output1 = in_Sensor1 * 2;
    }

    // Вызывается при изменении значения датчика
    function uniset_on_sensor(id, value) {
        mylog.info("Sensor " + id + " changed to " + value);
        // Реакция на изменение конкретного датчика
        if( id == Sensor1 )
        {
            ...
        }
    }
    \endcode

    \subsection subsec_jsTimers Система таймеров
    Модуль uniset2-timers.js предоставляет расширенные возможности работы с таймерами:

    \code
    function uniset_on_start() {
        // Одноразовый таймер
        askTimer("single", 5000, 1, function(id, count) {
            mylog.info("Single timer executed");
        });

        // Периодический таймер (3 раза)
        askTimer("repeat", 1000, 3, function(id, count) {
            mylog.info("Repeating timer: " + count);
        });

        // Бесконечный таймер
        askTimer("infinite", 2000, -1, function(id, count) {
            mylog.info("Heartbeat: " + count);
        });
    }

    function uniset_on_stop() {
        // Очистка всех таймеров
        clearAllTimers();
    }
    \endcode

    \subsection subsec_jsLogging Система логирования
    Модуль uniset2-log.js предоставляет логирование в стиле uniset

    \code
    // Создание логгера
    mylog = uniset_log_create("MyModule", true, true, true, true);

    // Настройка уровней логирования
    mylog.level("info", "warn", "crit", "level5");

    // Использование
    mylog.info("Information message");
    mylog.warn("Warning message");
    mylog.crit("Critical message");
    mylog.log5("Debug message level 5");
    \endcode


    \subsection subsec_jsUiObject Глобальный объект ui
    JScript предоставляет глобальный объект \b ui для прямого взаимодействия с системой uniset:

    \code
    // Функции объекта ui:
    ui.askSensor(sensor, command)    - Подписка/отписка на получение значений датчика
    ui.getValue(sensor)              - Чтение текущего значения датчика
    ui.setValue(sensor, value)       - Запись значения в датчик
    \endcode

    \note Причём в качестве \b sensor можно использовать \b идентификатор или \b имя датчика

    Доступные константы для команды подписки:
    - \b UIONotify - получать все значения
    - \b UIODontNotify - не получать значения (отписка)
    - \b UIONotifyChange - получать значения только при изменении
    - \b UIONotifyFirstNotNull - получать только первое ненулевое значение

    Пример использования
    \code
    function uniset_on_step() {
        // Чтение значения датчика по его ID
        let currentValue = ui.getValue(Sensor1);

        // Использование прочитанного значения
        if (currentValue > 100) {
            out_Output1 = 1;
        }

        // Чтение значения по заранее известному ID
        let temp = ui.getValue(12345);
        mylog.info("Temperature: " + temp);
    }
    \endcode


    \subsection subsec_jsAdvanced Расширенные возможности

    \subsubsection subsubsec_jsModules Модульная система
    Поддерживается загрузка модулей через функцию load() с поиском по указанным путям:
    \code
    // Загрузка модулей из различных путей
    load("uniset2-timers.js");
    load("uniset2-log.js");
    load("my_custom_module.js");
    load("utils/helpers.js");
    \endcode

    Доступные uniset-модули

    - uniset2_timers.js - таймеры \ref subsec_jsTimers
    - uniset2_log.js - логи \ref subsec_jsLogging
    - uniset2_delaytimer.js - аналог uniset::DelayTimer
    - uniset2_passivetimer.js - аналог uniset::PassiveTimer

    Базовый шаблон для написания свой программы main.js

    \section sec_jsConfig Конфигурация

    \subsection subsec_jsXML XML-конфигурация
    \code
    <JSProxy name="JSProxy1">
        <modules>
            <module path="/usr/share/uniset2/js"/>
            <module path="/custom/js/modules"/>
        </modules>
    </JSProxy>
    \endcode

    \subsection subsec_jsParams Параметры командной строки
    - \--js-name <name> - имя объекта в конфигурации
    - \--js-logfile <file> - файл для сохранения логов
    - \--js-sleep-msec <msec> - пауза между вызовами uniset_on_step (по умолчанию 150 msec)
*/
// --------------------------------------------------------------------------
#include "JSProxy_SK.h"
#include "JSEngine.h"
// --------------------------------------------------------------------------
namespace uniset
{
    // ----------------------------------------------------------------------
    class JSProxy final:
        public JSProxy_SK
    {
        public:
            explicit JSProxy( uniset::ObjectId id
                              , xmlNode* confnode = uniset::uniset_conf()->getNode("JSProxy")
                              , const std::string& prefix = "js" );
            ~JSProxy() override;

            static std::shared_ptr<JSProxy> init( int argc, const char* const* argv, const std::string& prefix = "js" );
            static void help_print();

        protected:

            virtual void callback() noexcept override;
            virtual void step() override;
            virtual void sysCommand(const uniset::SystemMessage* sm) override;
            virtual bool deactivateObject() override;
            virtual void askSensors( UniversalIO::UIOCommand cmd) override;
            virtual void sensorInfo(const uniset::SensorMessage* sm) override;
            //            virtual std::string getMonitInfo() const override;

        private:
            std::shared_ptr<JSEngine> js;
    };
    // ----------------------------------------------------------------------
} // end of namespace uniset
// --------------------------------------------------------------------------
#endif
