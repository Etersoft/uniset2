#if DISABLE_GO
#else

BUILD_SRCDIR=$(srcdir)/../core
BUILD_SRC=UProxy.cc

GO_PACKAGE_NAME=uniset_internal_api
GO_SRC_DIR=src/uniset_internal_api


BUILT_SOURCES = UInterface_wrap.cxx

UInterface_wrap.cxx: UInterface.i goUInterface.h $(BUILD_SRCDIR)/UProxy.cc $(BUILD_SRCDIR)/UProxy.h
	swig -go -cgo -intgosize 64 -c++ -outcurrentdir -I$(BUILD_SRCDIR) UInterface.i

# -use-shlib

lib_LTLIBRARIES = libUniSet2GO.la

libUniSet2GO_la_SOURCES    = goUInterface.cc  UProxy.cc UInterface_wrap.cxx
libUniSet2GO_la_CXXFLAGS   = -I$(BUILD_SRCDIR) $(UNISET_CFLAGS) $(UNISET_EXT_CFLAGS) $(PYTHON_CFLAGS)
libUniSet2GO_la_LDFLAGS    = -module -avoid-version
libUniSet2GO_la_LIBADD     = $(UNISET_LIBS) $(UNISET_EXT_LIBS) $(PYTHON_LIBS)

${GO_PACKAGE_NAME}.go: libUniSet2GO.la


${GO_SRC_DIR}/${GO_PACKAGE_NAME}.go: ${GO_PACKAGE_NAME}.go
	mkdir -p ${GO_SRC_DIR}
	cp -f ${GO_PACKAGE_NAME}.go ${GO_SRC_DIR}/${GO_PACKAGE_NAME}.go
	subst "s|#define intgo swig_intg|#cgo LDFLAGS: -L\$$\{SRCDIR\} -lUniSet2GO\n#define intgo swig_intg|g" ${GO_SRC_DIR}/${GO_PACKAGE_NAME}.go
	ln -sf .libs/libUniSet2GO.so ./
	ln -sf ../../.libs/libUniSet2GO.so ${GO_SRC_DIR}/

test:
	go build mtest.go

# Специальное правило для удаления сгенерированного кода
# для дальнейшей перегенерации
clean-gen:
	rm -rf ${GO_PACKAGE_NAME}.go *_wrap.cxx *_gc.c src

$(BUILD_SRC):
	ln -sf $(BUILD_SRCDIR)/$@

all-local: ${GO_SRC_DIR}/${GO_PACKAGE_NAME}.go


include $(top_builddir)/include.mk

#endif
