default: all

.DEFAULT:
	cd src && $(MAKE) $@
	cd tests && $(MAKE) $@
