#specify your compiler
CC	= g++

#first set up the platform dependent variables
#you could also add another ifeq entry for your own platform
ifeq ("$(shell uname)", "Darwin")
  #Note that these paths are based on installing OpenImageIO with
  #homebrew, hence there are no needs for the include paths
  OIIO_INC = 
  OIIO_LIB = -lOpenImageIO
  OPENGL_LIB = -framework Foundation -framework GLUT -framework OpenGL
else 
  ifeq ("$(shell uname)", "Linux")
    #On SoC machines, we need to specifically include the OIIO include
    #path in addition to the lib paths
    OIIO_INC = -I/group/dpa/include
    OIIO_LIB = -L/group/dpa/lib -lOpenImageIO
    OPENGL_LIB = -lglut -lGL -lGLU
  endif
endif

#build the LDFLAG snd CFLAGS based on variables above
LDFLAGS = $(OPENGL_LIB) $(OIIO_LIB)
CFLAGS = -g $(OIIO_INC)


histogram : histogram.o
	${CC} -o histogram histogram.o ${LDFLAGS}

histogram.o : histogram.cpp
	${CC} -c ${CFLAGS} histogram.cpp


#this will clean up all temporary files created by make all
clean:
	rm -f core.* *.o histogram
