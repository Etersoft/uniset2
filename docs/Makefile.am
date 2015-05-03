############################################################################
# This file is part of the UniSet library								  #
############################################################################

DOC_CFG_USER = *Dox.cfg
DOC_CFG_DEVEL = *DoxDevel.cfg

DIRS = html
DOCFILES = ${DIRS}/*.{html,css,png,gif}
########################################################################

if HAVE_DOXYGEN
all: user
user: ${DOC_CFG_USER}
	${DOXYGEN} $<
devel: ${DOC_CFG_DEVEL}
	${DOXYGEN} $<

depend :

clean: clean-local

clean-local:
	${RM} ${DOCFILES} *~

re: clean all


# install
ddir = $(docdir)/html
d_DATA = html/*

# examples
# exampledir = $(docdir)/examples
# example_DATA = examples/*

endif
