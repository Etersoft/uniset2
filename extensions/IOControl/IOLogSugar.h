#ifndef IOLogSugar_H_
#define IOLogSugar_H_
      // "синтаксический сахар"..для логов
		#ifndef ioinfo
			#define ioinfo if( iolog->debugging(Debug::INFO) ) iolog->info()
        #endif
		#ifndef iowarn
			#define iowarn if( iolog->debugging(Debug::WARN) ) iolog->warn()
        #endif
		#ifndef iocrit
			#define iocrit if( iolog->debugging(Debug::CRIT) ) iolog->crit()
        #endif
        #ifndef iolog1
        	#define iolog1 if( iolog->debugging(Debug::LEVEL1) ) iolog->level1()
        #endif
        #ifndef iolog2
	        #define iolog2 if( iolog->debugging(Debug::LEVEL2) ) iolog->level2()
        #endif
        #ifndef iolog3
    	    #define iolog3 if( iolog->debugging(Debug::LEVEL3) ) iolog->level3()
        #endif
        #ifndef iolog4
        	#define iolog4 if( iolog->debugging(Debug::LEVEL4) ) iolog->level4()
        #endif
        #ifndef iolog5
	        #define iolog5 if( iolog->debugging(Debug::LEVEL5) ) iolog->level5()
        #endif
        #ifndef iolog6
    	    #define iolog6 if( iolog->debugging(Debug::LEVEL6) ) iolog->level6()
        #endif
        #ifndef iolog7
        	#define iolog7 if( iolog->debugging(Debug::LEVEL7) ) iolog->level7()
        #endif
        #ifndef iolog8
	        #define iolog8 if( iolog->debugging(Debug::LEVEL8) ) iolog->level8()
        #endif
        #ifndef iolog9
    	    #define iolog9 if( iolog->debugging(Debug::LEVEL9) ) iolog->level9()
        #endif
        #ifndef iologany
        	#define iologany iolog->any()
        #endif
#endif
