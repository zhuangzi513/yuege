LOCAL := /home/ramo/Documents/yue/yuege/FileWrapper

CPP_SRC := \
    $(LOCAL)/text/TextXLSReader.cpp \

OBJS := $(CPP_SRC:.cpp=.o)

CPP_FLAG := \
    -I$(LOCAL)      \
    -I$(LOCAL)/text \
    $(NULL)

BASIC_LD_FLAG := -L/usr/lib/x86_64-linux-gnu/ -L/usr/lib -L/usr/lib/gcc/x86_64-linux-gnu/4.6/

LD_FLAG := $(BASIC_LD_FLAG) -lstdc++

TARGET:=libFileWrapper.so

$(TARGET):$(OBJS)
	echo $(OBJS)
	#$(LD) $< -o $@ $(LD_FLAG)
ifdef TEST
	$(MAKE)  $(LOCAL)/test/
endif

$(OBJS):%.o:%.cpp
	$(CC) -c $< -o $@  $(CPP_FLAG)

clean:
	rm -f $(OBJS) $(TARGET)
