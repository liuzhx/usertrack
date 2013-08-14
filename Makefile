all:usertracking usertracking_h3c
objects = usertracking.o usertracking_function.o tracemac.o
objects_h3c = usertracking_h3c.o usertracking_function.o tracemac.o 
usertracking:$(objects)
	cc  -g -o  usertracking $(objects)  -lsnmp -lconfig -lpthread `/usr/bin/mysql_config --libs --include`  -lrrd
usertracking_h3c:$(objects_h3c)
	 cc  -g -o  usertracking_h3c $(objects_h3c)  -lsnmp -lconfig -lpthread `/usr/bin/mysql_config --libs --include`  -lrrd
usertracking.o:usertracking.h
	cc -c -g -o usertracking.o usertracking.c  -lrrd
usertracking_h3c.o:usertracking.h
	cc -c -g -o usertracking_h3c.o usertracking_h3c.c  -lrrd
usertracking_function.o:usertracking.h
tracemac.o:usertracking.h
.PHONY:clean
clean:
	rm  usertracking usertracking_h3c usertracking.o  $(objects_h3c)
