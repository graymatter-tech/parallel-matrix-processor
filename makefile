COMPILER = mpicc
CFLAGS = -Wall
EXES = torus mkRandomMatrix mkIdentityMatrix mkZeroMatrix getMatrix
CFILES = I R RI IR
all: ${EXES}


getMatrix:   getMatrix.c matrix.o
	${COMPILER} ${CFLAGS} getMatrix.c matrix.o -o getMatrix

mkIdentityMatrix:    mkIdentityMatrix.c  matrix.o
	${COMPILER} ${CFLAGS} mkIdentityMatrix.c matrix.o -o mkIdentityMatrix

mkRandomMatrix:    mkRandomMatrix.c  matrix.o
	${COMPILER} ${CFLAGS} mkRandomMatrix.c matrix.o -o mkRandomMatrix


mkZeroMatrix:    mkZeroMatrix.c  matrix.o
	${COMPILER} ${CFLAGS} mkZeroMatrix.c matrix.o -o mkZeroMatrix

%.o: %.c %.h  makefile
	${COMPILER} ${CFLAGS} $< -c 

m_neighbours: m_neighbours.c matrix.o
	${COMPILER} ${CFLAGS} m_neighbours.c matrix.o -o m_neighbours

clean:
	rm -f *.o *~ ${EXES} ${CFILES}

