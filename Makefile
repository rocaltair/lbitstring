PLATFORM=$(shell uname)
CC = gcc
AR = ar

STATIC_LIB =  lbs.a
SHARED_LIB = lbs.so
OBJS = lbitstring.o

LLIBS = -llua
CFLAGS = -c -O3 -Wall -fPIC
LDFLAGS = -O3 -Wall --shared

ifeq ($(PLATFORM),Linux)
else
	ifeq ($(PLATFORM), Darwin)
		LLIBS += 
	endif
endif

all : $(SHARED_LIB) $(STATIC_LIB)

$(SHARED_LIB): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LLIBS)

$(STATIC_LIB): $(OBJS)
	$(AR) crs $@ $^

$(OBJS) : %.o : %.c
	$(CC) -o $@ $(CFLAGS) $<

clean : 
	rm -f $(OBJS) $(SHARED_LIB) $(STATIC_LIB)

.PHONY : clean

