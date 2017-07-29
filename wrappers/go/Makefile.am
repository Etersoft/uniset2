#if DISABLE_GO
#else

BUILD_SRCDIR=$(srcdir)/../core
BUILD_SRC=UProxyObject.cc

UInterface_wrap.cxx: UInterface.i goUInterface.h
	swig -go -cgo -intgosize 64 -c++ -outcurrentdir -I$(BUILD_SRCDIR) UInterface.i

# -use-shlib

lib_LTLIBRARIES = libgoUniSet.la

libgoUniSet_la_SOURCES    = goUInterface.cc UInterface_wrap.cxx UProxyObject.cc
libgoUniSet_la_CXXFLAGS   = -I$(BUILD_SRCDIR) $(UNISET_CFLAGS) $(UNISET_EXT_CFLAGS) $(PYTHON_CFLAGS)
libgoUniSet_la_LDFLAGS    = -module -avoid-version
libgoUniSet_la_LIBADD     = $(UNISET_LIBS) $(UNISET_EXT_LIBS) $(PYTHON_LIBS)


uniset.go: libgoUniSet.la


src/local-uniset/local-uniset.go: uniset.go
	mkdir -p src/local-uniset
	cp -f uniset.go src/local-uniset/local-uniset.go
	subst "s|#define intgo swig_intg|#cgo LDFLAGS: -L\$$\{SRCDIR\} -lgoUniSet\n#define intgo swig_intg|g" src/local-uniset/local-uniset.go
	ln -sf .libs/libgoUniSet.so ./
	ln -sf ../../.libs/libgoUniSet.so src/local-uniset/

test:
	go build mtest.go

# Специальное правило для удаления сгенерированного кода
# для дальнейшей перегенерации
clean-gen:
	rm -rf uniset.go *_wrap.cxx *_gc.c src

$(BUILD_SRC):
	ln -sf $(BUILD_SRCDIR)/$@

all-local: src/local-uniset/local-uniset.go


include $(top_builddir)/include.mk

#endif
