include $(BASE)/Makefile.inc

S = etc_init.d_tcprelay
E = $(INSTALLROOT)$(LIBARCHDIR_DEST)/mavis/extra

all: $(S)

$(S): ../../spawnd/extra/etc_init.d_spawnd
	-sed 's/spawnd/tcprelay/g'< $^ > $@

$(E)/$(S): $(S)
	$(INSTALL) -m 0755 $< $@

$(E):
	@mkdir -p -m 0755 $@

install: $(E) $(E)/$(S)

