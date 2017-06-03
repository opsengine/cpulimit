default: all

.DEFAULT:
	cd src && $(MAKE) $@
	cd tests && $(MAKE) $@

install:
	sudo cp src/cpulimit /usr/local/bin
