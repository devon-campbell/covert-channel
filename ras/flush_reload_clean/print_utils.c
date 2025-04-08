#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

static void print_stream(char c, const char *line_prefix){
    printf("%c", c);
    print_idx++;

    if (print_idx % 32 == 0)
    {
        printf("\n");
        if (line_prefix != NULL)
        {
            printf("%s", line_prefix);
        }
    }
    else if (print_idx % 4 == 0)
    {
        printf(" ");
    }

    fflush(stdout);
}

static void print_bit(bool bit, const char *line_prefix){
    if (bit)
        print_stream('1', line_prefix);
    else
        print_stream('0', line_prefix);
}