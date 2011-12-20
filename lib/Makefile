MODULES += base64/base64.o mail/mail.o
OBJECTS = base64.o mail.o

build_in.o: $(MODULES)
	$(AR) -r -o $@ $(OBJECTS) >/dev/null 2>&1

.c.o:
	$(CC) $(CFLAGS) -c $<  


clean:
	@rm -f *~
	@rm -f *.o
