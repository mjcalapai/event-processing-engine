_DEPS = log_entry.h severity_classifier.h boundedBuffer.h event_engine.h producer.h consumer.h mlfq_scheduler.h metrics.h
_OBJ = log_entry.o severity_classifier.o event_engine.o producer.o consumer.o boundedBuffer.o mlfq_scheduler.o metrics.o
_MOBJ = main.o
_TOBJ = test.o

APPBIN = event_engine_app
TESTBIN = event_engine_test

DEBUG = -DDEBUGMODE

IDIR = include
CC = g++
CFLAGS = -I$(IDIR) -Wall $(DEBUG) -Wextra -g -pthread
ODIR = obj
SDIR = src
LDIR = lib
TDIR = test
LIBS = -lm
XXLIBS = $(LIBS) -lstdc++ -lgtest -lgtest_main -lpthread

DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))
MOBJ = $(patsubst %,$(ODIR)/%,$(_MOBJ))
TOBJ = $(patsubst %,$(ODIR)/%,$(_TOBJ))

$(ODIR)/%.o: $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/%.o: $(TDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(APPBIN) $(TESTBIN) submission

$(APPBIN): $(OBJ) $(MOBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

$(TESTBIN): $(TOBJ) $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(XXLIBS)

submission:
	find . -name "*~" -exec rm -rf {} \;
	zip -r submission src lib include Makefile

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(IDIR)/*~
	rm -f $(APPBIN) $(TESTBIN)
	rm -f submission.zip