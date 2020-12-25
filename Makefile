all: ipd cli

ipd: ipd.c
	gcc -o ipd ipd.c
	
cli: cli.c
	gcc -o cli cli.c
	
clean:
	rm ./cli
	rm ./ipd

	
	
