DEVNAME=atahd

all: $(DEVNAME).o
#ide_core.o: ide_core.c ide_queue.c ide_ata.c ide_atapi.c ide_misc.c
#	$(CC) -c -o ide.o $<

objs=ide_core.o ide_queue.o ide_ata.o ide_atapi.o ide_misc.o ide_aix.o aix_svr4_shims.o atadebug_flex.o
$(DEVNAME).o: $(objs)
	ld -r -o $@ $(objs)


clean:
	-rm *.o

# Compiler command line for Metaware High C
CC=cc -DUSE_OS_LONG_IO

# Compiler command line for GCC
#CC=gcc -Wall -Wno-comment -Werror

# TODO: sort out the issues with optimized builds, then add
#  -O2

#cc -I. -D_KERNEL -DSYSV -DSVR40 -DAT386 -DVPIX -DWEITEK -DMERGE386 -DBLTCONS -DEVGA -c ide_core.c
# 12 cc -I. -D_KERNEL -DSYSV -DSVR40 -DAT386 -DVPIX -DWEITEK -DMERGE386 -DBLTCONS -DEVGA -c ide_queue.c
# 13 cc -I. -D_KERNEL -DSYSV -DSVR40 -DAT386 -DVPIX -DWEITEK -DMERGE386 -DBLTCONS -DEVGA -c ide_ata.c
# 14 cc -I. -D_KERNEL -DSYSV -DSVR40 -DAT386 -DVPIX -DWEITEK -DMERGE386 -DBLTCONS -DEVGA -c ide_atapi.c
# 15 cc -I. -D_KERNEL -DSYSV -DSVR40 -DAT386 -DVPIX -DWEITEK -DMERGE386 -DBLTCONS -DEVGA -c ide_misc.c
# 16 ld -r -o Driver.o ide_core.o ide_queue.o ide_ata.o ide_atapi.o ide_misc.o


# Common options
CFLAGS=-D_KERNEL -DKERNEL -Di386 -I/usr/include/sys
#-Iinclude -I.

uninstall:
	(cd /usr/sys/386 && ar -rv atlib.a hd.o)
	/usr/sys/newkernel -install

install: $(DEVNAME).o
	echo Saving out old hd.o
	if [ ! -f /usr/sys/386/hd.o ]; then cd /usr/sys/386 && ar -x atlib.a hd.o && ls -l hd.o;  fi
	echo Archiving new $(DEVNAME).o into the kernel library...
	ar -rv /usr/sys/386/atlib.a $(bin)$(DEVNAME).o
	# TODO
	#echo Installing hd support in master, system and predefined files
	#sh ./instal
	echo Rebuilding the kernel...
	/usr/sys/newkernel -install

