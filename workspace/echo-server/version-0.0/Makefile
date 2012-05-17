FLAG=-g
TARGET=echo_server echo_client

all:echo_server echo_client

debug:FLAG=-g -DDEBUG
debug: cleanall echo_server echo_client
	
echo_server: echo_server.cpp
	g++ -o $@ $(FLAG) $<

echo_client: echo_client.cpp
	g++ -o $@ $(FLAG) $<

cleanall:
	rm -rf $(TARGET)

r:cleanall all
