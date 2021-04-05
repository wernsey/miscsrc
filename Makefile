CC=gcc
CFLAGS= -c -Wall
LDFLAGS= -lm

# Add your source files here:
LIB_SOURCES=csv.c eval.c getarg.c hash.c ini.c list.c regex.c simil.c utils.c gc.c refcnt.c json.c
LIB_OBJECTS=$(LIB_SOURCES:.c=.o)
LIB=libmisc.a

DOCS=$(LIB_SOURCES:%.c=docs/%.html) docs/readme.html

TEST_SOURCES=test/test_csv.c test/test_eval.c test/test_arg.c test/test_hash.c test/test_list.c test/test_rx.c test/test_sim.c test/test_ini.c test/test_json.c
TEST_OBJECTS=$(TEST_SOURCES:%.c=%.o)

ifeq ($(BUILD),debug)
# Debug
CFLAGS += -O0 -g
LDFLAGS +=
else
# Release mode
CFLAGS += -O2 -DNDEBUG
LDFLAGS += -s
endif

# Detect operating system:
# More info: http://stackoverflow.com/q/714100
ifeq ($(OS),Windows_NT)
  EXE=.exe
  TESTS=$(TEST_SOURCES:%.c=%.exe)
else
  TESTS=$(TEST_SOURCES:%.c=%)
endif

all: $(LIB) $(TESTS) doc

lib: $(LIB)

doc : ./docs $(DOCS)

debug:
	make BUILD=debug

$(LIB): $(LIB_OBJECTS)
	ar rs $@ $^

.c.o:
	$(CC) $(CFLAGS) $< -o $@

csv.o: csv.c csv.h utils.h
eval.o: eval.c eval.h
hash.o: hash.c hash.h
ini.o: ini.c ini.h utils.h
list.o: list.c list.h
simil.o: simil.c simil.h
regex.o: regex.c regex.h
gc.o: gc.c gc.h
json.o: json.c json.h

# Test programs: Compile .o to executable
$(TEST_OBJECTS):
	$(CC) $(CFLAGS) -o $@ $<

$(TESTS):
	$(CC) -o $@ $^ $(LDFLAGS)

# Test program dependencies
test/test_arg.o: test/test_arg.c getarg.h
test/test_csv.o: test/test_csv.c csv.h
test/test_eval.o: test/test_eval.c eval.h
test/test_hash.o: test/test_hash.c hash.h
test/test_ini.o: test/test_ini.c ini.h
test/test_list.o: test/test_list.c list.h utils.h
test/test_rx.o: test/test_rx.c regex.h
test/test_sim.o: test/test_sim.c simil.h
test/test_json.o: test/test_json.c json.h

test/test_arg$(EXE): test/test_arg.o getarg.o
test/test_csv$(EXE): test/test_csv.o csv.o utils.o
test/test_eval$(EXE): test/test_eval.o eval.o
test/test_hash$(EXE): test/test_hash.o hash.o
test/test_ini$(EXE): test/test_ini.o ini.o utils.o
test/test_list$(EXE): test/test_list.o list.o utils.o
test/test_rx$(EXE): test/test_rx.o regex.o
test/test_sim$(EXE): test/test_sim.o simil.o
test/test_json$(EXE): test/test_json.o json.o

docs:
	mkdir -p docs

docs/%.html: %.h d.awk
	awk -f d.awk -vTitle=$< $< > $@

docs/readme.html: README.md d.awk
	awk -f d.awk -v Clean=1 -vTitle=$< $< > $@

.PHONY : clean

clean:
	-rm -f *.o test/*.o $(LIB)
	-rm -f $(TESTS) *.exe test/*.exe
	-rm -rf docs

# The .exe above is for MinGW, btw.