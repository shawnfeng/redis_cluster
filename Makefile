all: base_lib online

base_lib:
	cd src && $(MAKE)

online: base_lib
	cd online && $(MAKE)


test: base_lib
	cd test && $(MAKE)



.PHONY: clean cleanall
clean:
	cd src && $(MAKE) $@
	cd test && $(MAKE) $@
	cd online && $(MAKE) $@
	cd logic_driver && $(MAKE) $@
cleanall:
	cd src && $(MAKE) $@
	cd test && $(MAKE) $@
	cd online && $(MAKE) $@
	cd logic_driver && $(MAKE) $@
