CC = gcc

all: mmu process sched master
run: master
	./master
mmu: mmu.c
process: process.c
sched: sched.c
master: master.c

clean:
	rm -f mmu sched process master result.txt
