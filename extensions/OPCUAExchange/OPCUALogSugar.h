#ifndef OPCUASugar_H_
#define OPCUASugar_H_
// "синтаксический сахар"..для логов
#ifndef opcinfo
#define opcinfo if( opclog->debugging(Debug::INFO) ) opclog->info()
#endif
#ifndef opcwarn
#define opcwarn if( opclog->debugging(Debug::WARN) ) opclog->warn()
#endif
#ifndef opccrit
#define opccrit if( opclog->debugging(Debug::CRIT) ) opclog->crit()
#endif
#ifndef opclog1
#define opclog1 if( opclog->debugging(Debug::LEVEL1) ) opclog->level1()
#endif
#ifndef opclog2
#define opclog2 if( opclog->debugging(Debug::LEVEL2) ) opclog->level2()
#endif
#ifndef opclog3
#define opclog3 if( opclog->debugging(Debug::LEVEL3) ) opclog->level3()
#endif
#ifndef opclog4
#define opclog4 if( opclog->debugging(Debug::LEVEL4) ) opclog->level4()
#endif
#ifndef opclog5
#define opclog5 if( opclog->debugging(Debug::LEVEL5) ) opclog->level5()
#endif
#ifndef opclog6
#define opclog6 if( opclog->debugging(Debug::LEVEL6) ) opclog->level6()
#endif
#ifndef opclog7
#define opclog7 if( opclog->debugging(Debug::LEVEL7) ) opclog->level7()
#endif
#ifndef opclog8
#define opclog8 if( opclog->debugging(Debug::LEVEL8) ) opclog->level8()
#endif
#ifndef opclog9
#define opclog9 if( opclog->debugging(Debug::LEVEL9) ) opclog->level9()
#endif
#ifndef opclogany
#define opclogany opclog->any()
#endif
#endif
