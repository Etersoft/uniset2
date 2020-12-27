#ifndef UWebSocketGateSugar_H_
#define UWebSocketGateSugar_H_
// "синтаксический сахар"..
#ifndef myinfo
#define myinfo if( mylog->debugging(Debug::INFO) ) mylog->info()
#endif
#ifndef mywarn
#define mywarn if( mylog->debugging(Debug::WARN) ) mylog->warn()
#endif
#ifndef mycrit
#define mycrit if( mylog->debugging(Debug::CRIT) ) mylog->crit()
#endif
#ifndef mylog1
#define mylog1 if( mylog->debugging(Debug::LEVEL1) ) mylog->level1()
#endif
#ifndef mylog2
#define mylog2 if( mylog->debugging(Debug::LEVEL2) ) mylog->level2()
#endif
#ifndef mylog3
#define mylog3 if( mylog->debugging(Debug::LEVEL3) ) mylog->level3()
#endif
#ifndef mylog4
#define mylog4 if( mylog->debugging(Debug::LEVEL4) ) mylog->level4()
#endif
#ifndef mylog5
#define mylog5 if( mylog->debugging(Debug::LEVEL5) ) mylog->level5()
#endif
#ifndef mylog6
#define mylog6 if( mylog->debugging(Debug::LEVEL6) ) mylog->level6()
#endif
#ifndef mylog7
#define mylog7 if( mylog->debugging(Debug::LEVEL7) ) mylog->level7()
#endif
#ifndef mylog8
#define mylog8 if( mylog->debugging(Debug::LEVEL8) ) mylog->level8()
#endif
#ifndef mylog9
#define mylog9 if( mylog->debugging(Debug::LEVEL9) ) mylog->level9()
#endif
#ifndef mylogany
#define mylogany mylog->any()
#endif
#endif
