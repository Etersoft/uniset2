# Интерфейс на python

В проект входит несколько python-интерфейсов

## Интерфейс для работы с uniset

В данном интерфейсе реализованы только самые просты функции \b getValue и \b setValue и некоторые вспомогательные,
позволяющие получать дополнительную информацию по датчикам.

\sa [UConnector](\ref UConnector)
	
Пример использования:
```
#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys

from uniset import *

if __name__ == "__main__":
	
	lst = Params_inst()

	for i in range(0, len(sys.argv)):
		if i >= Params.max:
			break;
		lst.add( sys.argv[i] )

	try:	
		uc = UConnector( lst, "test.xml" )
		
		print "(0)UIType: %s" % uc1.getUIType()
		print "(1)getShortName: id=%d name=%s" % (1, uc.getShortName(1))
		print "     getName: id=%d name=%s" % (1, uc.getName(101))
		print " getTextName: id=%d name=%s" % (1, uc.getTextName(101))
		print "\n"
		print "getShortName: id=%d name=%s" % (2, uc.getShortName(109))
		print "     getName: id=%d name=%s" % (2, uc.getName(109))
		print " getTextName: id=%d name=%s" % (2, uc.getTextName(109))

		try:
			print "(1)setValue: %d=%d" % (3,22)
			uc.setValue(3,22,DefaultID)
		except UException, e:
			print "(1)setValue exception: " + str(e.getError())

		try:
			print "(1)getValue: %d=%d" % ( 3, uc.getValue(3,DefaultID) )
		except UException, e:
			print "(1)getValue exception: " + str(e.getError())

	except UException, e:
		print "(testUI): catch exception: " + str(e.getError())
```

## Интерфейс для работы с modbus
В данном интерфейсе реализованы функции Modbus master на основе использования libuniset.
Он имеет ряд простых функций getWord(), getByte(), getBit(), а так же универсальная функция
UModbus::mbread(...) позволяющая более тонко определять параметры запроса.

Для записи одного регистра реализована UModbus::mbwrite()

\sa UModbus

Пример использования:
```
#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys

from uniset import *

if __name__ == "__main__":

    try:
        mb = UModbus()
        
        print "UIType: %s" % mb.getUIType()
        
        mb.connect("localhost",2048)
        try:
            print "Test READ functions..."
            for f in range(1,5):
                print "func=%d reg=%d" % (f,22)
                val = mb.mbread(0x01,22,f,"unsigned",-1)
#               val = mb.mbread(0x01,22)
                print "val=%d"%val
            
            print "getWord: %d" % mb.getWord(0x01,22)
            print "getByte: %d" % mb.getByte(0x01,22)
            print "getBit: %d" % mb.getBit(0x01,22,3)
        except UException, e:
            print "exception: " + str(e.getError())

    except UException, e:
         print "(testUModbus): catch exception: " + str(e.getError())
```

## Работа c заказом датчиков
В данной реализации в фоне (в отдельном от питон системном потоке) создаётся proxy-объект,
который заказывает датчики и асинхронно в фоне получает уведомления об их изменении.
При этом из python-программы путём периодического опроса при помощи getValue() можно значения
забирать. Такой режим работы позволяет не делать каждый раз "удалённые вызовы" для получения 
значений датчиков, обновление будет происходить асинхронно в фоновом потоке.

При этом после создания такого объекта, необходимо при помощи функции `addToAsk(id)` добавить
интересующие датчики, после чего для активации объекта необходимо вызвать функцию
`uniset_activate_objects()`

**warning:** функция `uniset_activate_objects()` должна быть вызвана только один раз в начале работы программы.

Пример:
```
#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
from uniset import *

if __name__ == "__main__":

    # command line arguments
    lst = Params_inst()

    for i in range(0, len(sys.argv)):
        if i >= Params.max:
            break;
        lst.add(sys.argv[i])

    try:
        uniset_init_params(lst, "test.xml");

        # create proxxy-object
        obj1 = UProxyObject("TestProc")
        
        # ask sensors
        obj1.addToAsk(1)
        obj1.addToAsk(10)
        
        # run proxy objects
        uniset_activate_objects()

        # work..
        while True:
            print "get current value..."
            print "getValue: sensorID=%d value=%d" % (10, getValue(10))
            time.sleep(2) # do some work

    except UException, e:
        print "(testUI): catch exception: " + str(e.getError())

```
