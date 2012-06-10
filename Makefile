CC=g++
RM=rm
OBJS=lattice.o pair.o lm.o converter.o
MAIN_OBJ=converter_main.o
TEST_OBJ=converter_test.o
LIBS=-lmarisa
LIBS_TEST=-lgtest -lgtest_main

VPATH=src

converter-main: $(OBJS) $(MAIN_OBJ)
	$(CC) -o $@ $(OBJS) $(MAIN_OBJ) $(LIBS)

test: $(OBJS) $(TEST_OBJ)
	$(CC) -o $@ $(OBJS) $(TEST_OBJ) $(LIBS) $(LIBS_TEST)
	./test

.PHONY:clean
clean:
	$(RM) *.o

.cpp.o:
	$(CC) -c $< $(LIBS)

converter.o: converter.h lm.h pair.h lattice.h
converter_main.o: converter.h lm.h pair.h
converter_test.o: converter.h lm.h pair.h
lattice.o: lattice.h lm.h pair.h
lm.o: lm.h pair.h
pair.o: lm.h pair.h
