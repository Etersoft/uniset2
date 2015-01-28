if DISABLE_PYTHON
else
python_SCRIPTS = pyUniSet.py pyUConnector.py pyUModbus.py pyUExceptions.py


pyUniSet.py: UInterface_wrap.cxx _pyUniSet.la 
	

pyUConnector.py: UConnector_wrap.cxx _pyUConnector.la 
	

pyUModbus.py: _pyUModbus.la UModbus_wrap.cxx
	

pyUExceptions.py: _pyUExceptions.la
	

pyexec_LTLIBRARIES = _pyUConnector.la _pyUModbus.la _pyUExceptions.la _pyUniSet.la

_pyUniSet_la_SOURCES 	= PyUInterface.cc UInterface_wrap.cxx
_pyUniSet_la_CXXFLAGS	= $(UNISET_CFLAGS) $(PYTHON_CFLAGS)
_pyUniSet_la_LDFLAGS 	= -module -avoid-version
_pyUniSet_la_LIBADD 	= $(UNISET_LIBS) $(PYTHON_LIBS)

UInterface_wrap.cxx: UInterface.i PyUInterface.h
	swig -python -c++ -classic UInterface.i

_pyUConnector_la_SOURCES 	= UConnector.cc UConnector_wrap.cxx
_pyUConnector_la_CXXFLAGS	= $(UNISET_CFLAGS) $(PYTHON_CFLAGS) 
_pyUConnector_la_LDFLAGS 	= -module -avoid-version
_pyUConnector_la_LIBADD 	= $(UNISET_LIBS) $(PYTHON_LIBS)

UConnector_wrap.cxx: UConnector.i UConnector.h
	swig -python -c++ -classic UConnector.i

_pyUModbus_la_SOURCES 	= UModbus.cc UModbus_wrap.cxx
_pyUModbus_la_CXXFLAGS	= $(UNISET_CFLAGS) $(UNISET_EXT_CFLAGS) $(COMCPP_CFLAGS) $(PYTHON_CFLAGS) 
_pyUModbus_la_LDFLAGS 	= -module -avoid-version
_pyUModbus_la_LIBADD 	= $(UNISET_LIBS) $(UNISET_EXT_LIBS) $(COMCPP_LIBS) $(PYTHON_LIBS)

UModbus_wrap.cxx: UInterface.i UModbus.h
	swig -python -c++ -classic UModbus.i

_pyUExceptions_la_SOURCES 	= UExceptions_wrap.cxx
_pyUExceptions_la_CXXFLAGS	= $(UNISET_CFLAGS) $(UNISET_EXT_CFLAGS) $(PYTHON_CFLAGS) 
_pyUExceptions_la_LDFLAGS 	= -module -avoid-version
_pyUExceptions_la_LIBADD 	= $(UNISET_LIBS) $(UNISET_EXT_LIBS) $(PYTHON_LIBS)

UExceptions_wrap.cxx: UExceptions.i UExceptions.h
	swig -python -c++ -classic UExceptions.i

clean-local:
	rm -rf *.py* *_wrap.cxx

endif
