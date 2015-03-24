all:
	$(CC) client.c -o controller_client
	$(CC) server.c -g ml_model.c -o controller_server -lpthread

clean:
	-rm controller_client controller_server
