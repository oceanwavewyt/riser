#OBJNAME=libgentle.a
include ./makefile.pub

$(OUTPUT_NAME_SERVER):$(OUTPUT_SERVER_OBJ)
	$(CC) -o $@ $^ $(LIB_PATHS) $(LDFLAGS) -rdynamic
$(OUTPUT_NAME_CLIENT):$(OUTPUT_CLIENT_OBJ)
	$(CC) -o $@ $^ $(LIB_PATHS) $(CLDFLAGS) -DCLIENT -rdynamic
