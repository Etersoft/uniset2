if ENABLE_ST2JS

st2jsdir = $(datadir)/@PACKAGE@/st2js/st2js
st2js_DATA = st2js/__init__.py \
	st2js/__main__.py \
	st2js/cli.py \
	st2js/codegen.py \
	st2js/errors.py \
	st2js/fb_registry.py \
	st2js/ir.py \
	st2js/mapping.py \
	st2js/parser.py \
	st2js/transformer.py \
	st2js/type_system.py

st2js_metadir = $(datadir)/@PACKAGE@/st2js
st2js_meta_DATA = pyproject.toml README.md

# install wrapper script
bin_SCRIPTS = uniset2-st2js

include $(top_builddir)/include.mk

endif
