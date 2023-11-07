# gcc (GCC) 11.4.0
# x86_64-w64-mingw32-gcc (GCC) 11.4.0

#CC = gcc
CC = x86_64-w64-mingw32-gcc
TARBALL = InfoGB_05J

# InfoGB
.CFILES =	../cpu.cpp \
		../joypad.cpp \
		../mem.cpp \
		../rom.cpp \
		../sound.cpp \
		../vram.cpp \
		./win32.cpp \
		./dirsound.cpp

.OFILES	=	$(.CFILES:.cpp=.o)

CCFLAGS = 
LDFILGS = -lcomdlg32 -lstdc++ -lwinmm -lgdi32 -ldsound

all: InfoGB

InfoGB: $(.OFILES)
	$(CC) $(INCLUDES) -o $@ $(.OFILES) $(LDFILGS) -lm

.cpp.o:
	$(CC) $(INCLUDES) -c $(CCFLAGS) $*.cpp  -o $@

clean:
	rm -f $(.OFILES) ./*~ ../*~ *.stackdump

cleanall:
	rm -f $(.OFILES) ./*~ ../*~ *.stackdump InfoGB.exe

release: clean all

tar:
	( cd ..; \
	tar cvf $(TARBALL).tar ./*; \
	gzip $(TARBALL).tar \
	)

install:
	install ./InfoGB /usr/local/bin
