#ifndef DBLogSugar_H_
#define DBLogSugar_H_
      // "синтаксический сахар"..для логов
		#ifndef dbinfo
			#define dbinfo if( dblog->debugging(Debug::INFO) ) dblog->info()
        #endif
		#ifndef dbwarn
			#define dbwarn if( dblog->debugging(Debug::WARN) ) dblog->warn()
        #endif
		#ifndef dbcrit
			#define dbcrit if( dblog->debugging(Debug::CRIT) ) dblog->crit()
        #endif
        #ifndef dblog1
        	#define dblog1 if( dblog->debugging(Debug::LEVEL1) ) dblog->level1()
        #endif
        #ifndef dblog2
	        #define dblog2 if( dblog->debugging(Debug::LEVEL2) ) dblog->level2()
        #endif
        #ifndef dblog3
    	    #define dblog3 if( dblog->debugging(Debug::LEVEL3) ) dblog->level3()
        #endif
        #ifndef dblog4
        	#define dblog4 if( dblog->debugging(Debug::LEVEL4) ) dblog->level4()
        #endif
        #ifndef dblog5
	        #define dblog5 if( dblog->debugging(Debug::LEVEL5) ) dblog->level5()
        #endif
        #ifndef dblog6
    	    #define dblog6 if( dblog->debugging(Debug::LEVEL6) ) dblog->level6()
        #endif
        #ifndef dblog7
        	#define dblog7 if( dblog->debugging(Debug::LEVEL7) ) dblog->level7()
        #endif
        #ifndef dblog8
	        #define dblog8 if( dblog->debugging(Debug::LEVEL8) ) dblog->level8()
        #endif
        #ifndef dblog9
    	    #define dblog9 if( dblog->debugging(Debug::LEVEL9) ) dblog->level9()
        #endif
        #ifndef dblogany
        	#define dblogany dblog->any()
        #endif
#endif // end of DBLogSugar
