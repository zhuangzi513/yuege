LOCAL := /home/ramo/Documents/yue/yuege/Build

CPP_SRC := \
    $(LOCAL)/DBBuilder.cpp \
    $(LOCAL)/sqlite/sqlite3.c

OBJS := $(CPP_SRC:.cpp=.o)

CPP_FLAG := \
    -I$(LOCAL)        \
    -I$(LOCAL)/sqlite \
    $(NULL)

LD_FLAG := -lstlport

TARGET:=libDBBuilder.so

$(TARGET):$(OBJS)
	echo $(OBJS)
	#$(LD) $< -o $@ $(LD_FLAG)
ifdef TEST
	$(MAKE)  $(LOCAL)/test/
endif

$(OBJS):%.o:%.cpp
	$(CC) -c $< -o $@  $(CPP_FLAG)

clean:
	rm $(OBJS)
