#include <stdint.h>
#include <stdio.h>
#include <string.h>

void hexdump(const char *prefix, const void *buf, int size)
{
    const uint8_t *b = (const uint8_t *) buf;
    for (int i = 0; i < size; i += 16) {
        if (prefix != NULL && strlen(prefix) > 0) {
            printf("%s %04x: ", prefix, i);
        }
        for (int j = 0; j < 16; j++) {
            if ((i + j) < size) {
                printf("%02x", b[i + j]);
            } else {
                printf("  ");
            }
            if ((j + 1) % 2 == 0) {
                putchar(' ');
            }
        }
        putchar(' ');
        for (int j = 0; j < 16 && (i + j) < size; j++) {
            putchar(b[i + j] >= 0x20 && b[i + j] <= 0x7E ? b[i + j] : '.');
        }
        printf("\r\n");
    }
}
