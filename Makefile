arguments = -I /usr/local/include/json-c/ -I./ -L/usr/local/lib64 -lssl -lcrypto -lpthread -ljson-c -Wno-int-conversion
common = comm.c connection.c util.c
client_source = client/client_comm.c client/client_main.c client/client_net.c
server_source = server/server_net.c server/server_main.c server/server_cmd.c server/server_comm.c server/server_broad.c
test_source = test/test.c test/test_json.c test/test_random.c


all: $(common) proc_client proc_server proc_recv


proc_recv: $(common) client/client_broad.c client/client_net.c client/client_comm.c
	gcc -g -o proc_recv $(common) client/client_net.c client/client_broad.c client/client_comm.c $(arguments)


proc_client: $(common) $(client_source)
	gcc -g -o proc_client $(common) $(client_source) $(arguments)


proc_server: $(common) $(server_source)
	gcc -g -o proc_server $(common) $(server_source) $(arguments)


test: $(common) $(test_source)
	gcc -g -o proc_test $(common) test/test.c $(arguments)
	gcc -g -o proc_test_json $(common) test/test_json.c $(arguments)
	gcc -g -o proc_test_random $(common) test/test_random.c $(arguments)


clean:
	rm -f test/*.o
	rm -f test/proc_*
	rm -f *.o
	rm -f proc_*