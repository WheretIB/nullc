config=debug

REG_CFLAGS=-g -Wall -Wextra -Wno-unused-variable -std=c++14 -I../external

ifeq ($(config),release)
	REG_CFLAGS += -O3 -fno-omit-frame-pointer -DNDEBUG
endif

SERVER_OBJECTS = temp/nullc_lang_server/handler.o temp/nullc_lang_server/main.o
DEBUGGER_OBJECTS = temp/nullc_lang_debugger/debug.o temp/nullc_lang_debugger/handler.o temp/nullc_lang_debugger/main.o

all: temp/.dummy temp/nullc_lang_server/.dummy temp/nullc_lang_debugger/.dummy nullc_lang_server nullc_lang_debugger

temp/nullc_lang_server/%.o: nullc_lang_server/%.cpp
	$(CXX) $(REG_CFLAGS) -c $< -o $@

temp/nullc_lang_debugger/%.o: nullc_lang_debugger/%.cpp
	$(CXX) $(REG_CFLAGS) -c $< -o $@

temp/.dummy:
	mkdir -p temp
	touch temp/.dummy

temp/nullc_lang_server/.dummy:
	mkdir -p temp/nullc_lang_server
	touch temp/nullc_lang_server/.dummy
	
temp/nullc_lang_debugger/.dummy:
	mkdir -p temp/nullc_lang_debugger
	touch temp/nullc_lang_debugger/.dummy

clean:
	rm -rf temp/
	rm nullc_lang_client/out/nullc_lang_server
	rm nullc_lang_client/out/nullc_lang_debugger

nullc_lang_server: ${SERVER_OBJECTS}
	$(CXX) $(REG_CFLAGS) -o nullc_lang_client/out/$@ $(SERVER_OBJECTS) -L../bin -lnullc -ldl -lstdc++ -lm

nullc_lang_debugger: ${DEBUGGER_OBJECTS}
	$(CXX) $(REG_CFLAGS) -o nullc_lang_client/out/$@ $(DEBUGGER_OBJECTS) -L../bin -lnullc -ldl -lstdc++ -lm -lpthread
