#include <endian.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>


int main(int argc, char *argv[])
{
    if (argc < 2) {
	fprintf(stderr, "Usage: ./fld_loc_double_arr_dump DS_PATH\n");
	return -1;
    }
    FILE *f = fopen(argv[1], "w");
    if (!f) {
	perror("fopen");
	return -1;
    }

    for (int i = 0; i < 2; i++) {
	for (int j = 0; j < 3; j++) {
	    uint8_t bar_len = j;
	    fwrite(&bar_len, sizeof(bar_len), 1, f);
	    for (int k = 0; k < bar_len; k++) {
		uint8_t ele = k;
		fwrite(&ele, sizeof(ele), 1, f);
	    }
	}
    }

    fclose(f);

    return 0;
}
