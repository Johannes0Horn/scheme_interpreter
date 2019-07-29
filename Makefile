.PHONY: clean

scheme: scheme.c
		gcc -o scheme scheme.c
clean:
	rm scheme