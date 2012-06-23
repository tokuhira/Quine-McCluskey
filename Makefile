CC	 = clang++
CFLAGS	 = -Wall -g -std=c++0x 
LDFLAGS	 =
INCLUDES = -I${BOOST_DIR}/include/
LIBS	 = -L${BOOST_DIR}/lib -lboost_regex
TARGET	 = a.out
OBJS	 = main.o

all:	$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	-rm -f $(TARGET) $(OBJS) .nfs* *~ \#* core

.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<
