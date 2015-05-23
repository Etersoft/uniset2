#ifndef UNETLogSugar_H_
#define UNETLogSugar_H_
// "синтаксический сахар"..для логов
#ifndef unetinfo
#define unetinfo if( unetlog->debugging(Debug::INFO) ) unetlog->info()
#endif
#ifndef unetwarn
#define unetwarn if( unetlog->debugging(Debug::WARN) ) unetlog->warn()
#endif
#ifndef unetcrit
#define unetcrit if( unetlog->debugging(Debug::CRIT) ) unetlog->crit()
#endif
#ifndef unetlog1
#define unetlog1 if( unetlog->debugging(Debug::LEVEL1) ) unetlog->level1()
#endif
#ifndef unetlog2
#define unetlog2 if( unetlog->debugging(Debug::LEVEL2) ) unetlog->level2()
#endif
#ifndef unetlog3
#define unetlog3 if( unetlog->debugging(Debug::LEVEL3) ) unetlog->level3()
#endif
#ifndef unetlog4
#define unetlog4 if( unetlog->debugging(Debug::LEVEL4) ) unetlog->level4()
#endif
#ifndef unetlog5
#define unetlog5 if( unetlog->debugging(Debug::LEVEL5) ) unetlog->level5()
#endif
#ifndef unetlog6
#define unetlog6 if( unetlog->debugging(Debug::LEVEL6) ) unetlog->level6()
#endif
#ifndef unetlog7
#define unetlog7 if( unetlog->debugging(Debug::LEVEL7) ) unetlog->level7()
#endif
#ifndef unetlog8
#define unetlog8 if( unetlog->debugging(Debug::LEVEL8) ) unetlog->level8()
#endif
#ifndef unetlog9
#define unetlog9 if( unetlog->debugging(Debug::LEVEL9) ) unetlog->level9()
#endif
#ifndef unetlogany
#define unetlogany unetlog->any()
#endif
#endif // end of UNETLogSugar
