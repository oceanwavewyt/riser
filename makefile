#OBJNAME=libgentle.a
OBJNAME=gentle

include ./makefile.pub

$(OBJNAME):$(objects)
	$(CC) -o $(OBJNAME) $(objects) $(LIB_PATHS) $(LDFLAGS) -rdynamic
