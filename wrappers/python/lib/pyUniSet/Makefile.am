if DISABLE_PYTHON
else

SWIG=swig

#python_SCRIPTS = pyUniSet.py pyUConnector.py pyUModbus.py pyUExceptions.py
pydir=$(pkgpythondir)
py_DATA = pyUniSet.py pyUConnector.py pyUModbus.py pyUExceptions.py

BUILD_SRCDIR=$(srcdir)/../../../core
BUILD_SRC=UModbus.cc UConnector.cc UProxyObject.cc

pyUniSet.py: UInterface_wrap.cxx _pyUniSet.la


pyUConnector.py: UConnector_wrap.cxx _pyUConnector.la


pyUModbus.py: _pyUModbus.la UModbus_wrap.cxx


pyUExceptions.py: _pyUExceptions.la


pyexec_LTLIBRARIES = _pyUConnector.la _pyUModbus.la _pyUExceptions.la _pyUniSet.la

_pyUniSet_la_SOURCES 	= PyUInterface.cc UInterface_wrap.cxx UProxyObject.cc
_pyUniSet_la_CXXFLAGS	= -I$(BUILD_SRCDIR) $(UNISET_CFLAGS) $(UNISET_EXT_CFLAGS) $(PYTHON_CFLAGS)
_pyUniSet_la_LDFLAGS 	= -module -avoid-version
_pyUniSet_la_LIBADD 	= $(UNISET_LIBS) $(UNISET_EXT_LIBS) $(PYTHON_LIBS)

UInterface_wrap.cxx: UInterface.i UProxyObject.i PyUInterface.h $(BUILD_SRCDIR)/UProxyObject.h
	$(SWIG) -python -c++ -classic -outcurrentdir -I$(BUILD_SRCDIR) UInterface.i

#PyUObject_wrap.cxx: UObject.i PyUObject.h
#	swig -python -c++ -classic UObject.i

_pyUConnector_la_SOURCES 	= UConnector.cc UConnector_wrap.cxx
_pyUConnector_la_CXXFLAGS	= -I$(BUILD_SRCDIR) $(UNISET_CFLAGS) $(PYTHON_CFLAGS)
_pyUConnector_la_LDFLAGS 	= -module -avoid-version
_pyUConnector_la_LIBADD 	= $(UNISET_LIBS) $(PYTHON_LIBS)

UConnector_wrap.cxx: UConnector.i $(BUILD_SRCDIR)/UConnector.h
	$(SWIG) -python -c++ -classic -outcurrentdir -I$(BUILD_SRCDIR) UConnector.i

_pyUModbus_la_SOURCES 	= UModbus.cc UModbus_wrap.cxx
_pyUModbus_la_CXXFLAGS	= -I$(BUILD_SRCDIR) $(UNISET_CFLAGS) $(UNISET_EXT_CFLAGS) $(POCO_CFLAGS) $(PYTHON_CFLAGS)
_pyUModbus_la_LDFLAGS 	= -module -avoid-version
_pyUModbus_la_LIBADD 	= $(UNISET_LIBS) $(UNISET_EXT_LIBS) $(POCO_LIBS) $(PYTHON_LIBS)

UModbus_wrap.cxx: UInterface.i $(BUILD_SRCDIR)/UModbus.h
	$(SWIG) -python -c++ -classic -outcurrentdir -I$(BUILD_SRCDIR) UModbus.i

_pyUExceptions_la_SOURCES 	= UExceptions_wrap.cxx
_pyUExceptions_la_CXXFLAGS	= -I$(BUILD_SRCDIR) $(UNISET_CFLAGS) $(UNISET_EXT_CFLAGS) $(PYTHON_CFLAGS)
_pyUExceptions_la_LDFLAGS 	= -module -avoid-version
_pyUExceptions_la_LIBADD 	= $(UNISET_LIBS) $(UNISET_EXT_LIBS) $(PYTHON_LIBS)

UExceptions_wrap.cxx: UExceptions.i $(BUILD_SRCDIR)/UExceptions.h
	$(SWIG) -python -c++ -classic -outcurrentdir -I$(BUILD_SRCDIR) UExceptions.i

# Специальное правило для удаления сгенерированного кода
# для дальнейшей перегенерации
clean-gen:
	rm -rf *.py* *_wrap.cxx

$(BUILD_SRC):
	ln -sf $(BUILD_SRCDIR)/$@

all-local: $(BUILD_SRC)


include $(top_builddir)/include.mk

endif
