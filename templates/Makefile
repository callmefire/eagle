MODULES += template.o SP.o

build_in.o: $(MODULES)
	@$(AR) -r -o $@ $(MODULES) >/dev/null 2>&1
	@echo "AR $@"

.c.o:
	@$(CC) $(CFLAGS) -c $<  
	@echo "CC $@"

clean:
	@rm -f *~
	@rm -f *.o
