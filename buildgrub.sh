cp midnight.bin isodir/boot/
grub-mkrescue -o midnight.iso isodir
qemu-system-i386 -cdrom midnight.iso