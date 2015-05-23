#ifndef MBLogSugar_H_
#define MBLogSugar_H_
// "синтаксический сахар"..для логов
#ifndef mbinfo
#define mbinfo if( mblog->debugging(Debug::INFO) ) mblog->info()
#endif
#ifndef mbwarn
#define mbwarn if( mblog->debugging(Debug::WARN) ) mblog->warn()
#endif
#ifndef mbcrit
#define mbcrit if( mblog->debugging(Debug::CRIT) ) mblog->crit()
#endif
#ifndef mblog1
#define mblog1 if( mblog->debugging(Debug::LEVEL1) ) mblog->level1()
#endif
#ifndef mblog2
#define mblog2 if( mblog->debugging(Debug::LEVEL2) ) mblog->level2()
#endif
#ifndef mblog3
#define mblog3 if( mblog->debugging(Debug::LEVEL3) ) mblog->level3()
#endif
#ifndef mblog4
#define mblog4 if( mblog->debugging(Debug::LEVEL4) ) mblog->level4()
#endif
#ifndef mblog5
#define mblog5 if( mblog->debugging(Debug::LEVEL5) ) mblog->level5()
#endif
#ifndef mblog6
#define mblog6 if( mblog->debugging(Debug::LEVEL6) ) mblog->level6()
#endif
#ifndef mblog7
#define mblog7 if( mblog->debugging(Debug::LEVEL7) ) mblog->level7()
#endif
#ifndef mblog8
#define mblog8 if( mblog->debugging(Debug::LEVEL8) ) mblog->level8()
#endif
#ifndef mblog9
#define mblog9 if( mblog->debugging(Debug::LEVEL9) ) mblog->level9()
#endif
#ifndef mblogany
#define mblogany mblog->any()
#endif
#endif
