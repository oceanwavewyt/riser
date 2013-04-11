#OBJNAME=libgentle.a
OBJNAME=riser

include ./makefile.pub

$(OBJNAME):$(objects)
	$(CC) -o $(OBJNAME) $(objects) $(LIB_PATHS) $(LDFLAGS) -rdynamic
