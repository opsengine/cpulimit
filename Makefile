default: all

.DEFAULT:
	$(MAKE) -C src $@
	$(MAKE) -C tests $@
