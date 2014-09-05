// -------------------------------------------------------------------------
#ifndef LogServerTypes_H_
#define LogServerTypes_H_
// -------------------------------------------------------------------------
#include <ostream>
// -------------------------------------------------------------------------
namespace LogServerTypes
{
    const unsigned int MAGICNUM = 0x20140904;
    enum Command
    {
        cmdNOP,            /*!< отсутствие команды */
        cmdSetLevel,    /*!< установить уровень вывода */
        cmdAddLevel,    /*!< добавить уровень вывода */
        cmdDelLevel,    /*!< удалить уровень вывода */
        cmdRotate,        /*!< пересоздать файл с логами */
        cmdOffLogFile,    /*!< отключить запись файла логов (если включена) */
        cmdOnLogFile    /*!< включить запись файла логов (если была отключена) */
        // cmdSetLogFile
    };

    std::ostream& operator<<(std::ostream& os, Command c );

    struct lsMessage
    {
        lsMessage():magic(MAGICNUM),cmd(cmdNOP),data(0){}
        unsigned int magic;
        Command cmd;
        unsigned int data;

        // для команды 'cmdSetLogFile'
        // static const short MAXLOGFILENAME = 100;
        // char logname[MAXLOGFILENAME];
    }__attribute__((packed));
    
    std::ostream& operator<<(std::ostream& os, lsMessage& m );
}
// -------------------------------------------------------------------------
#endif // LogServerTypes_H_
// -------------------------------------------------------------------------
