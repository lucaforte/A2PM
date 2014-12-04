all:
	$(CC) aggregate.c list.c -o ../ML-Framework/aggregate
	$(CC) leak.c -lm -o leaks
	$(CC) thread.c -lpthread -lm -o threads

clean:
	-rm ../ML-Framework/aggregate leaks threads
