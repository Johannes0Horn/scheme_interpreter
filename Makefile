.PHONY: clean

scheme: scheme.c
		gcc -g -o scheme scheme.c
clean:
	rm scheme