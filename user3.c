extern void __attribute__((regparm(1)))
putc (char c);

extern void __attribute__((regparm(1)))
hlt ();

extern void __attribute__((regparm(1)))
page2_fun ();

static void puts_1(const char *str) {
        if (str[0] != '\0') {
            putc(str[0]);
            puts_1(str+1);
        }
    }

void puts(const char *str) {
    puts_1(str);
    putc('\n');
}


int main() {
    puts("Here is user mode!!");

    puts("Let's call the function of PAGE2");

    page2_fun();

    puts("user EXIT!");

//    char *kernel_pa = (void *)0xF0002000;
//    putc(kernel_pa[0]);
    hlt();

}
