LDFLAGS = -lm
CFLAGS = -O3 -g -MD

fractalGenerator: fractalGenerator.o

-include *.d


clean:
	rm -f *.o *.d fractalGenerator

.PHONY: clean