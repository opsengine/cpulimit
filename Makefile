default: all

.DEFAULT:
	cd src && $(MAKE) $@
	cd test && $(MAKE) $@
