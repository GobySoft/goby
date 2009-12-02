.PHONY: subsystem
subsystem:
	cd simple && $(MAKE)
	cd two_message && $(MAKE)
	cd plusnet && $(MAKE)
	cd test && $(MAKE)

.PHONY: clean
clean:
	cd simple && $(MAKE) clean
	cd two_message && $(MAKE) clean
	cd plusnet && $(MAKE) clean
	cd test && $(MAKE) clean	
