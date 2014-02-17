DEFINES += -DTEST

TARGET:=DB
LD_FLAG:= -L.. -L/usr/lib/gcc/x86_64-linux-gnu/4.6/ -L/usr/lib -lpthread -ldl -lstdc++

FILEWRAPPER_SRC:=\
    ../../FileWrapper/text/TextXLSReader.cpp \
    $(NULL)

FILEWRAPPER_INCLUDE:=\
    -I../../FileWrapper      \
    -I../../FileWrapper/text \
    $(NULL)


CPP_FLAG:=\
    $(FILEWRAPPER_INCLUDE)   \
    -I/usr/include/          \
    -I../                    \
    -I../../common           \
    -I../sqlite              \
    $(NULL)

CPP_FLAG+=\
    -DTEST=1

CPP_SRC:=\
    $(FILEWRAPPER_SRC)    \
    main.cpp              \
    ../DBFilter.cpp       \
    ../DBWrapper.cpp      \
    ../OriginDBHelper.cpp \
    $(NULL)

FILEWRAPPER_SRC:=\
    ../../FileWrapper/text/TextXLSReader.cpp \
    $(NULL)

C_SRC:=\
    ../sqlite/sqlite3.c


CPP_OBJS:=$(CPP_SRC:.cpp=.o)
C_OBJS+=$(C_SRC:.c=.o)

$(TARGET):$(CPP_OBJS) $(C_OBJS)
	echo $(CPP_OBJS) $(C_OBJS)
	$(CC) $(CPP_OBJS) $(C_OBJS) -g -o $@ $(LD_FLAG) $(CPP_FLAG)

$(CPP_OBJS):%.o:%.cpp
	echo $(CPP_OBJS)
	$(CC) -c -g $< -o $@ $(LD_FLAG) $(CPP_FLAG)

$(C_OBJS):%.o:%.c
	echo $(C_OBJS)
	$(CC) -c -g $< -o $@ $(LD_FLAG) $(CPP_FLAG)

clean:
	rm -f $(TARGET) *.db *log* core

distclean:
	rm -f $(CPP_OBJS) $(C_OBJS) $(TARGET) *.db

