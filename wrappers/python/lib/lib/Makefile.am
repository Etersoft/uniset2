if ENABLE_PYTHON

SWIG=swig

#python_SCRIPTS = pyUniSet.py pyUConnector.py pyUModbus.py pyUExceptions.py
pydir=$(pkgpythondir)/lib
py_DATA = __init__.py UniSet.py UConnector.py UModbus.py UExceptions.py

BUILD_SRCDIR=$(abs_top_srcdir)/wrappers/core
BUILD_SRC=UModbus.cc UConnector.cc UProxyObject.cc

UniSet.py: UInterface_wrap.cxx _UniSet.la

UConnector.py: UConnector_wrap.cxx _UConnector.la

UModbus.py: _UModbus.la UModbus_wrap.cxx

UExceptions.py: _UExceptions.la


pyexec_LTLIBRARIES = _UConnector.la _UModbus.la _UExceptions.la _UniSet.la

_UniSet_la_SOURCES 	= PyUInterface.cc UInterface_wrap.cxx UProxyObject.cc
_UniSet_la_CXXFLAGS	= -I$(BUILD_SRCDIR) $(UNISET_CFLAGS) $(UNISET_EXT_CFLAGS) $(PYTHON_CFLAGS)
_UniSet_la_LDFLAGS 	= -module -avoid-version
_UniSet_la_LIBADD 	= $(UNISET_LIBS) $(UNISET_EXT_LIBS) $(PYTHON_LIBS)

UInterface_wrap.cxx: UInterface.i UProxyObject.i PyUInterface.h $(BUILD_SRCDIR)/UProxyObject.h
	$(SWIG) -python -py3 -c++ -outcurrentdir -I$(BUILD_SRCDIR) $(abs_srcdir)/UInterface.i

#PyUObject_wrap.cxx: UObject.i PyUObject.h
#	swig -python -c++ -classic UObject.i

_UConnector_la_SOURCES 	= UConnector.cc UConnector_wrap.cxx
_UConnector_la_CXXFLAGS	= -I$(BUILD_SRCDIR) $(UNISET_CFLAGS) $(PYTHON_CFLAGS)
_UConnector_la_LDFLAGS 	= -module -avoid-version
_UConnector_la_LIBADD 	= $(UNISET_LIBS) $(PYTHON_LIBS)

UConnector_wrap.cxx: UConnector.i $(BUILD_SRCDIR)/UConnector.h
	$(SWIG) -python -py3 -c++ -outcurrentdir -I$(BUILD_SRCDIR) $(abs_srcdir)/UConnector.i

_UModbus_la_SOURCES 	= UModbus.cc UModbus_wrap.cxx
_UModbus_la_CXXFLAGS	= -I$(BUILD_SRCDIR) $(UNISET_CFLAGS) $(UNISET_EXT_CFLAGS) $(POCO_CFLAGS) $(PYTHON_CFLAGS)
_UModbus_la_LDFLAGS 	= -module -avoid-version
_UModbus_la_LIBADD 	= $(UNISET_LIBS) $(UNISET_EXT_LIBS) $(POCO_LIBS) $(PYTHON_LIBS)

UModbus_wrap.cxx: UInterface.i $(BUILD_SRCDIR)/UModbus.h
	$(SWIG) -python -py3 -c++ -outcurrentdir -I$(BUILD_SRCDIR) $(abs_srcdir)/UModbus.i

_UExceptions_la_SOURCES 	= UExceptions_wrap.cxx
_UExceptions_la_CXXFLAGS	= -I$(BUILD_SRCDIR) $(UNISET_CFLAGS) $(UNISET_EXT_CFLAGS) $(PYTHON_CFLAGS)
_UExceptions_la_LDFLAGS 	= -module -avoid-version
_UExceptions_la_LIBADD 	= $(UNISET_LIBS) $(UNISET_EXT_LIBS) $(PYTHON_LIBS)

UExceptions_wrap.cxx: UExceptions.i $(BUILD_SRCDIR)/UExceptions.h
	$(SWIG) -python -py3 -c++ -outcurrentdir -I$(BUILD_SRCDIR) $(abs_srcdir)/UExceptions.i

# Специальное правило для удаления сгенерированного кода
# для дальнейшей перегенерации
clean-gen:
	rm -rf *.py* *_wrap.cxx

$(BUILD_SRC):
	ln -sf $(BUILD_SRCDIR)/$@

all-local: $(BUILD_SRC)


include $(top_builddir)/include.mk

endif
