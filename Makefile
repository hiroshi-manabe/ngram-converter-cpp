CXX		?= g++

OBJS		:= obj/lattice.o obj/pair.o obj/lm.o obj/converter.o
MAIN_OBJ	:= obj/converter_main.o
TEST_OBJ	:= obj/converter_test.o

MAIN_CMD	:= converter-main
TEST_CMD	:= test

CXXFLAGS	?= -Wall -O0 -g
CPPFLAGS	+= $(shell pkg-config --cflags marisa)
LDFLAGS		+= $(shell pkg-config --libs marisa)
CPPFLAGS_TEST	:= -I/usr/local/include
LDFLAGS_TEST	:= -L/usr/local/lib -lgtest -lgtest_main

# 「*_TEST」については、出来れば「pkg-config」で処理した方が良いです。

VPATH		:= src

obj/%.o : %.cpp
	@mkdir -p $(@D)
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $(CPPFLAGS_TEST) $< -o $@

$(MAIN_CMD): $(OBJS) $(MAIN_OBJ)
	$(CXX) $^ $(LDFLAGS) -o $@

$(TEST_CMD): $(OBJS) $(TEST_OBJ)
	$(CXX) $^ $(LDFLAGS) $(LDFLAGS_TEST) -o $@
#	./$(TEST_CMD)

.PHONY: clean

clean:
	rm -f $(OBJS) $(MAIN_OBJ) $(TEST_OBJ)
	for d in $(sort $(dir $(OBJS) $(MAIN_OBJ) $(TEST_OBJ))); do \
		if [ -d $$d ]; then rmdir -p $$d || echo "Skipping non-empty directory"; fi \
	done
	rm -f $(MAIN_CMD) $(TEST_CMD)

obj/converter.o: converter.h lm.h pair.h lattice.h
obj/converter_main.o: converter.h lm.h pair.h
obj/converter_test.o: converter.h lm.h pair.h
obj/lattice.o: lattice.h lm.h pair.h
obj/lm.o: lm.h pair.h
obj/pair.o: lm.h pair.h
