#ifndef HttpResolverSugar_H_
#define HttpResolverSugar_H_
// "синтаксический сахар"..для логов
#ifndef rinfo
#define rinfo if( rlog->debugging(Debug::INFO) ) rlog->info()
#endif
#ifndef rwarn
#define rwarn if( rlog->debugging(Debug::WARN) ) rlog->warn()
#endif
#ifndef rcrit
#define rcrit if( rlog->debugging(Debug::CRIT) ) rlog->crit()
#endif
#ifndef rlog1
#define rlog1 if( rlog->debugging(Debug::LEVEL1) ) rlog->level1()
#endif
#ifndef rlog2
#define rlog2 if( rlog->debugging(Debug::LEVEL2) ) rlog->level2()
#endif
#ifndef rlog3
#define rlog3 if( rlog->debugging(Debug::LEVEL3) ) rlog->level3()
#endif
#ifndef rlog4
#define rlog4 if( rlog->debugging(Debug::LEVEL4) ) rlog->level4()
#endif
#ifndef rlog5
#define rlog5 if( rlog->debugging(Debug::LEVEL5) ) rlog->level5()
#endif
#ifndef rlog6
#define rlog6 if( rlog->debugging(Debug::LEVEL6) ) rlog->level6()
#endif
#ifndef rlog7
#define rlog7 if( rlog->debugging(Debug::LEVEL7) ) rlog->level7()
#endif
#ifndef rlog8
#define rlog8 if( rlog->debugging(Debug::LEVEL8) ) rlog->level8()
#endif
#ifndef rlog9
#define rlog9 if( rlog->debugging(Debug::LEVEL9) ) rlog->level9()
#endif
#ifndef rlogany
#define rlogany rlog->any()
#endif
#endif
