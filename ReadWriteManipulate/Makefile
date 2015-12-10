#specify your compiler
CC	= g++

OIIO_INC = -I/
OIIO_LIB = -lOpenImageIO
OPENGL_LIB = -lglut -lGL -lGLU

#build the LDFLAG snd CFLAGS based on variables above
LDFLAGS = $(OPENGL_LIB) $(OIIO_LIB)
CFLAGS = -g $(OIIO_INC)

#this will be the name of your executable
PROJECT = oiioview

#list a .o file for each .cpp file that you will compile
#this makefile will compile each cpp separately before linking
OBJECTS = oiioview.o 
#file1.o  

#this target does the linking step  
all: ${PROJECT}

${PROJECT}: ${OBJECTS}
	${CC} -o ${PROJECT} ${OBJECTS} ${LDFLAGS}

#this target generically compiles each .cpp to a .o file
#it does not check for .h files dependencies, but you could add that
%.o: %.cpp
	${CC} -c ${CFLAGS} *.cpp

#this will clean up all temporary files created by make all
clean:
	rm -f core.* *.o *~ ${PROJECT}
