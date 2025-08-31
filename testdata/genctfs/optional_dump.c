#include <endian.h>
#include <stdio.h>
#include <stdint.h>


int main(int argc, char *argv[])
{
    if (argc < 2) {
	fprintf(stderr, "Usage: ./bit_dump DS_PATH\n");
	return -1;
    }
    FILE *f = fopen(argv[1], "w");
    if (!f) {
	perror("fopen");
	return -1;
    }
    uint8_t val8;
    int8_t sval8;
    size_t ret;

    /* Selector field */
    val8 = 0;  // Enabled
    ret = fwrite(&val8, sizeof(val8), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }
    /* Optional field */
    sval8 = -10;
    ret = fwrite(&sval8, sizeof(sval8), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Selector field */
    val8 = 13;  // Disabled
    ret = fwrite(&val8, sizeof(val8), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }
    // Only field is disabled.

    fclose(f);

    return 0;
}
