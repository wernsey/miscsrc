CC=gcc
CFLAGS=-c -Wall
LDFLAGS=

# Add your source files here:
LIB_SOURCES=csv.c eval.c getarg.c hash.c ini.c list.c regex.c simil.c utils.c gc.c
LIB_OBJECTS=$(LIB_SOURCES:.c=.o)
LIB=libmisc.a

DOCS=$(LIB_SOURCES:%.c=docs/%.html) docs/readme.html

TEST_SOURCES=test/test_csv.c test/test_eval.c test/test_arg.c test/test_hash.c test/test_list.c test/test_rx.c test/test_sim.c test/test_ini.c
TEST_OBJECTS=$(TEST_SOURCES:.c=.o)
TESTS=$(TEST_SOURCES:%.c=%)

ifeq ($(BUILD),debug)
# Debug
CFLAGS += -O0 -g
LDFLAGS +=
else
# Release mode
CFLAGS += -O2 -DNDEBUG
LDFLAGS += -s
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

# Test programs: Compile .o to executable
test_%: test_%.o libmisc.a
	$(CC) $(LDFLAGS) -o $@ $^ -lm
	
# Test program dependencies
test_csv.o : csv.h
test_eval.o : eval.h
test_arg.o : getarg.h
test_hash.o : hash.h
test_list.o : list.h utils.h
test_rx.o : regex.h
test_sim.o : simil.h

docs :
	mkdir -p docs

docs/%.html : %.h doc.awk
	awk -f doc.awk -vtitle=$< $< > $@ 

docs/readme.html: README doc.awk
	awk -f doc.awk -vtitle=$< $< > $@ 
	
.PHONY : clean 

clean:
	-rm -f *.o $(LIB)
	-rm -f $(TESTS) *.exe test/*.exe
	-rm -rf docs
# The .exe above is for MinGW, btw.