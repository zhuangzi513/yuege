CC:=g++
LD:=ld

ifdef HOME
export SRC_DIR=/home/gaia/workspace/yuege
else
export SRC_DIR=/home/ramo/Documents/yue/yuege
endif

export OBJS_DIR=$(SRC_DIR)

CPP_DIR:= \
    $(SRC_DIR)/Build \
    $(SRC_DIR)/FileWrapper \
    $(NULL)

LD_FLAG:=

CPP_FLAG:=\
    -I$(SRC_DIR)/Build \
    -I$(SRC_DIR)/Build/sqlite \
    -I$(SRC_DIR)/FileWrapper \
    $(NULL)

TARGET:=test

$(TARGET):$(OBJS_DIR) main.o
	gcc main.cpp Build/DBFilter.cpp Build/sqlite/sqlite3.c -I./Build -I./Build/sqlite -L/usr/lib/x86_64-linux-gnu/ -L/usr/lib -L/usr/lib64 -L/usr/lib/gcc/x86_64-linux-gnu/4.6/ -lpthread -ldl -lxml2 -lstdc++
	#$(MAKE) -C $(SRC_DIR)/FileWrapper
	#$(MAKE) -C $(SRC_DIR)/Build
	#$(CC) main.cpp $(CPP_FLAG) $(LD_FLAG) -o bin

main.o:%.o:%.cpp
	$(CC) -c $< -o $(OBJS_DIR)/$@ $(CPP_FLAG) 
