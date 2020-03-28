OBJ2IMG = obj2img


TAPES = cmfl01.tap

all: $(TAPES)

clean:
	rm -f $(TAPES) *.img 

cmfl01.tap: $(CTSSBSS) mit8c0.ctssld
	ls -1 *.bss | awk -F . '{ print "obj2img -b -f " $$1 " -e " $$2 " -a \" m1416cmfl01\" -o " $$1 ".img " $$1 "."$$2 }' >/tmp/bssimg.sh
	@sh -x /tmp/bssimg.sh
	@rm -f /tmp/bssimg.sh
	$(OBJ2IMG) -t -f mit8c0 -e ctssld -a " M1416CMFL01" -o mit8c0.img mit8c0.ctssld
	cat *.img ../EOF.BIN >cmfl01.tap
