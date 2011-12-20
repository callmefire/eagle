MODULES += template.o SP.o

build_in.o: $(MODULES)
	$(AR) -r -o $@ $(MODULES) >/dev/null 2>&1

.c.o:
	$(CC) $(CFLAGS) -c $<  


clean:
	@rm -f *~
	@rm -f *.o
