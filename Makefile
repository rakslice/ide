DEVNAME=atahd

all: $(DEVNAME).o
#ide_core.o: ide_core.c ide_queue.c ide_ata.c ide_atapi.c ide_misc.c
#	$(CC) -c -o ide.o $<

objs=ide_core.o ide_queue.o ide_ata.o ide_atapi.o ide_misc.o ide_aix.o aix_svr4_shims.o atadebug_flex.o
$(DEVNAME).o: $(objs)
	ld -r -o $@ $(objs)

$(objs): ide.h aix_svr4_shims.h

clean:
	-rm *.o

# Compiler command line for Metaware High C
CC=cc -DUSE_OS_LONG_IO

# Compiler command line for GCC
#CC=gcc -Wall -Wno-comment -Werror

# TODO: sort out the issues with optimized builds, then add
#  -O2

# Common options
CFLAGS=-D_KERNEL -DKERNEL -Di386 -I/usr/include/sys
#-Iinclude -I.

uninstall:
	(cd /usr/sys/386 && ar -rv atlib.a hd.o)
	/usr/sys/newkernel -install

install: $(DEVNAME).o
	if [ ! -f /usr/sys/386/hd.o ]; then echo Saving out old hd.o; cd /usr/sys/386 && ar -x atlib.a hd.o && ls -l hd.o;  fi
	echo Archiving new $(DEVNAME).o into the kernel library...
	ar -rv /usr/sys/386/atlib.a $(bin)$(DEVNAME).o
	# TODO
	#echo Installing hd support in master, system and predefined files
	#sh ./instal
	echo Rebuilding the kernel...
	/usr/sys/newkernel -install

