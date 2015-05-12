CFLAGS=-g

all:
	$(CC) client.c $(CFLAGS) -o controller_client
	$(CC) server.c $(CFLAGS) ml_model.c -o controller_server -lpthread
	$(CC) balancer.c sockhelp.c thread.c atomic.c $(CFLAGS) -pthread -o balancer

clean:
	-rm controller_client controller_server balancer
