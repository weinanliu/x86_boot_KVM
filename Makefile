
# user1.S must be the first
USER_SRC = user1.S user2.S user3.c
USER_OBJ := $(patsubst %.c, %.o, $(USER_SRC))
USER_OBJ := $(patsubst %.S, %.o, $(USER_OBJ))

USER_CFLAGS = -m32 -nostdinc -fno-stack-protector
USER_LDFLAGS = -T user.ld

# kern.S must be the first
KERN_SRC = kern.S kern_c.c
KERN_OBJ := $(patsubst %.c, %.o, $(KERN_SRC))
KERN_OBJ := $(patsubst %.S, %.o, $(KERN_OBJ))

KERN_CFLAGS = -m32 -nostdinc -fno-stack-protector
KERN_LDFLAGS = -T kernel.ld

a.out: halo.c memdata.bin.o
	gcc -g halo.c memdata.bin.o

memdata.bin.o: kernel bios user
	objcopy -O binary -j .data bios memdata.bios.bin
	./pad_align.sh memdata.bios.bin
	objcopy -O binary -j .data kernel memdata.kernel.bin
	./pad_align.sh memdata.kernel.bin
	objcopy -O binary -j .data user memdata.user.bin
	./pad_align.sh memdata.user.bin
	ld -r -o memdata.bin.o -b binary memdata.bios.bin memdata.kernel.bin memdata.user.bin
	objcopy --set-section-alignment .data=4096 memdata.bin.o

user: $(USER_OBJ)
	ld -o $@ $(USER_LDFLAGS) $(USER_OBJ)

bios: bios.o
	ld -o $@ -T bios.ld bios.o

kernel: $(KERN_OBJ)
	ld -o $@ $(KERN_LDFLAGS) $(KERN_OBJ)

%.o: %.c
	gcc $(KERN_CFLAGS) -c -o $@ $<

%.o: %.S
	gcc $(KERN_CFLAGS) -c -o $@ $<

clean:
	rm *.o kernel bios a.out *.bin user

.PHONY: clean
