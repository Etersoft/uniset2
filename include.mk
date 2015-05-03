
style:
	afiles="$(shell find $(srcdir) -name '*.cc' -or -name '*.h')"; \
	if test -n "$$afiles"; then astyle $(ASTYLE_OPT) $$afiles; fi
