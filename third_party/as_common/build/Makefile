EXTENDS = /usr/local
ifneq ("as","as${with-extends}")
        EXTENDS =${with-extends}
endif
INSTALL_ROOT = $(EXTENDS)
ifneq ("as","as${prefix}")
        INSTALL_ROOT =${prefix}
endif

NAME = libcommon

C++             = g++
TARGET_DLL      = $(NAME).so
TARGET_STATIC   = $(NAME).a


OS_NAME = $(shell uname -p)

INCPATH   += -I../inc/

C++FLAGS  += -shared -lm -lpthread -pipe -fPIC -g -ldl -Wall -O0 -DENV_LINUX -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE

CPPFILES += $(wildcard ../src/*.cpp)
CFILES += $(wildcard ../src/*.c)
HEADFILES += $(wildcard ../inc/*.h)

COBJS=$(CFILES:.c=.o)
CPPOBJS=$(CPPFILES:.cpp=.O)


all: $(TARGET_DLL) $(TARGET_STATIC)
$(TARGET_DLL): $(CPPOBJS) $(COBJS)
	$(C++) $(C++FLAGS) -o $@ $(CPPOBJS) $(COBJS)
$(TARGET_STATIC): $(COBJS)
	$(AR) rcs $@ $(CPPOBJS) $(COBJS)
	
%.o : %.c $(HEADFILES)
	$(C++) -c  $(C++FLAGS) $(INCPATH) $< -o $@

%.O : %.cpp $(HEADFILES)
	$(C++) -c  $(C++FLAGS) $(INCPATH) $< -o $@	

clean:
	rm -f $(TARGET_DLL) $(TARGET_STATIC) $(CPPOBJS) $(COBJS)

install: 
	mkdir -p ./${OS_NAME}
	mkdir -p ${OS_NAME}/include/
	mkdir -p ${OS_NAME}/lib/
	cp ../inc/*.h   ${OS_NAME}/include/
	cp -d ./${TARGET_DLL} ${OS_NAME}/lib/
	cp -d ./${TARGET_STATIC} ${OS_NAME}/lib/

##### Any additional, platform-specific rules come here:
