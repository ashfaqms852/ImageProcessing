#specify your compiler
CC	= g++

OIIO_INC = -I/
OIIO_LIB = -lOpenImageIO
OPENGL_LIB = -lglut -lGL -lGLU

#build the LDFLAG snd CFLAGS based on variables above
LDFLAGS = $(OPENGL_LIB) $(OIIO_LIB)
CFLAGS = -g $(OIIO_INC)


composite : composite.o
	${CC} -o composite composite.o ${LDFLAGS}

composite.o : composite.cpp
	${CC} -c ${CFLAGS} composite.cpp


alphamask : alphamask.o
	${CC} -o alphamask alphamask.o ${LDFLAGS}

alphamask.o : alphamask.cpp
	${CC} -c ${CFLAGS} alphamask.cpp

all : composite alphamask


#this will clean up all temporary files created by make all
clean:
	rm -f core.* *.o composite alphamask
