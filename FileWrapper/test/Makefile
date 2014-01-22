DEFINES += -DTEST

TARGET:=text_xml
LD_FLAG:= -L.. -L/usr/lib/gcc/x86_64-linux-gnu/4.6/ -lstdc++

CPP_FLAG:=\
    -I../   \
    -I../text

CPP_FLAG+=\
    -DTEST=1

CPP_SRC:=\
    main.cpp \
    ../text/TextXLSReader.cpp

OBJS:=$(CPP_SRC:.cpp=.o)

$(TARGET):$(OBJS)
	echo $(OBJS)
	$(CC) $(OBJS) -o $@ $(LD_FLAG) $(CPP_FLAG)

$(OBJS):%.o:%.cpp
	echo $(OBJS)
	$(CC) -c $< -o $@ $(LD_FLAG) $(CPP_FLAG)

clean:
	rm -f $(OBJS) $(TARGET)
