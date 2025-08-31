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
    uint32_t val32;
    uint8_t val8;
    int8_t sval8;
    size_t ret;

    /* Selector field */
    val8 = 5;
    ret = fwrite(&val8, sizeof(val8), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }
    /* Variant field */
    sval8 = -5;
    ret = fwrite(&sval8, sizeof(sval8), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Selector field */
    val8 = 20;
    ret = fwrite(&val8, sizeof(val8), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    val32 = 0xdeadbeef;
    ret = fwrite(&val32, sizeof(val32), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    fclose(f);

    return 0;
}
