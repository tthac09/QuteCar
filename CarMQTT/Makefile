MAIN_TOPDIR = $(shell pwd)
export MAIN_TOPDIR

all:
	$(MAKE) -f non_factory.mk all

clean:
	$(MAKE) -f non_factory.mk clean

factory:
	$(MAKE) -f factory.mk all

clean_factory:
	$(MAKE) -f factory.mk clean

