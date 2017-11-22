default: all

.DEFAULT:
	cd src && $(MAKE) $@
	cd tests && $(MAKE) $@

.PHONY: install
install:
	cp src/cpulimit /usr/local/bin
