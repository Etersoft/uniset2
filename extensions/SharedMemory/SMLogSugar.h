#ifndef SMLogSugar_H_
#define SMLogSugar_H_
// "синтаксический сахар"..для логов
#ifndef sminfo
#define sminfo if( smlog->debugging(Debug::INFO) ) smlog->info()
#endif
#ifndef smwarn
#define smwarn if( smlog->debugging(Debug::WARN) ) smlog->warn()
#endif
#ifndef smcrit
#define smcrit if( smlog->debugging(Debug::CRIT) ) smlog->crit()
#endif
#ifndef smlog1
#define smlog1 if( smlog->debugging(Debug::LEVEL1) ) smlog->level1()
#endif
#ifndef smlog2
#define smlog2 if( smlog->debugging(Debug::LEVEL2) ) smlog->level2()
#endif
#ifndef smlog3
#define smlog3 if( smlog->debugging(Debug::LEVEL3) ) smlog->level3()
#endif
#ifndef smlog4
#define smlog4 if( smlog->debugging(Debug::LEVEL4) ) smlog->level4()
#endif
#ifndef smlog5
#define smlog5 if( smlog->debugging(Debug::LEVEL5) ) smlog->level5()
#endif
#ifndef smlog6
#define smlog6 if( smlog->debugging(Debug::LEVEL6) ) smlog->level6()
#endif
#ifndef smlog7
#define smlog7 if( smlog->debugging(Debug::LEVEL7) ) smlog->level7()
#endif
#ifndef smlog8
#define smlog8 if( smlog->debugging(Debug::LEVEL8) ) smlog->level8()
#endif
#ifndef smlog9
#define smlog9 if( smlog->debugging(Debug::LEVEL9) ) smlog->level9()
#endif
#ifndef smlogany
#define smlogany smlog->any()
#endif
#endif
