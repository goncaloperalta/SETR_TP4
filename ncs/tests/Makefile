run: test_cmd.o ./no_nfr/cmdproc.o ../unity/unity.o
	gcc test_cmd.c ./no_nfr/cmdproc.c ../unity/unity.c
	./a.out

clean:
	rm -f *.o
	rm -f no_nfr/*.o
	rm -f a.out
	rm -f ../unity/*.o