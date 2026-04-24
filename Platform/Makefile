all:: Platform.a

include ../common.mk

Linux/Linux.a:
	$(MAKE) -C Linux

Platform.a: Linux/Linux.a
	cp -uv $< $@

clean::
	$(MAKE) -C Linux clean
	rm -f Platform.a
