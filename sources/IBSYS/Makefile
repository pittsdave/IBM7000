IBSYSDIRS = 9PAC EDITOR FORTRAN IBCBC IBFTC IBJOB IBMAP IBSYS IOCS \
	RESTART SORT UTILS
EXTRADIRS = CT disk misc suppak

RTLDIRS = IBCBC/rtl IBFTC/rtl

all:
	@for dir in $(IBSYSDIRS) $(EXTRADIRS); do \
	   $(MAKE) -C $$dir; \
	done

clean:
	@for dir in $(IBSYSDIRS) $(EXTRADIRS); do \
	   $(MAKE) -C $$dir clean; \
	done

distclean: clean
	rm -rf checkdir

setcheck:
	@for dir in $(IBSYSDIRS) $(RTLDIRS) $(EXTRADIRS); do \
	   mkdir -p checkdir/$$dir; \
	   cp $$dir/*.obj checkdir/$$dir/; \
	done

check:
	@for dir in $(IBSYSDIRS) $(EXTRADIRS); do \
	   checkit $$dir; \
	done
	@for dir in $(RTLDIRS); do \
	   checkrtl $$dir; \
	done

