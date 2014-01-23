LOCAL := /home/ramo/Documents/yue/yuege/Build

CPP_SRC := \
    $(LOCAL)/DBFilter.cpp       \
    $(LOCAL)/DBWrapper.cpp      \
    $(LOCAL)/OriginDBHelper.cpp

C_SRC := \
    $(LOCAL)/sqlite/sqlite3.c

CPP_OBJS := $(CPP_SRC:.cpp=.o)
C_OBJS := $(C_SRC:.c=.o)
OBJS := $(CPP_OBJS) $(C_OBJS)

CPP_FLAG := \
    -I$(LOCAL)                     \
    -I$(LOCAL)/sqlite              \
    -I$(LOCAL)/../FileWrapper      \
    -I$(LOCAL)/../FileWrapper/text \
    -I$(LOCAL)/../common           \
    $(NULL)

BASIC_LD_FLAG := -L/usr/lib/x86_64-linux-gnu/ -L/usr/lib -L/usr/lib64 -L/usr/lib/gcc/x86_64-linux-gnu/4.6/
LD_FLAG := $(BASIC_LD_FLAG) -lpthread -ldl -lxml2 -lstdc++

TARGET:=libDBFilter.so

$(TARGET):$(OBJS)
	echo $(OBJS)
	#$(LD) $(OBJS) -o $@ $(LD_FLAG)
ifdef TEST
	$(MAKE)  $(LOCAL)/test/
endif

$(CPP_OBJS):%.o:%.cpp
	$(CC) -c $< -o $@  $(CPP_FLAG)

$(C_OBJS):%.o:%.c
	$(CC) -c $< -o $@  $(CPP_FLAG)

clean:
	rm $(CPP_OBJS) $(C_OBJS) $(TARGET)
