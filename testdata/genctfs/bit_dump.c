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
    uint32_t val_le, val_be;
    size_t ret;

    val_le = htole32(0xdeadbeef);
    ret = fwrite(&val_le, sizeof(val_le), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }
    val_be = htobe32(0xdeadbeef);
    ret = fwrite(&val_be, sizeof(val_be), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    val_le = htole32(0x1337cafe);
    ret = fwrite(&val_le, sizeof(val_le), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }
    val_be = htobe32(0x1337cafe);
    ret = fwrite(&val_be, sizeof(val_be), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    fclose(f);

    return 0;
}
