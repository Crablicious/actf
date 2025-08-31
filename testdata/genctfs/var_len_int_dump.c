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

    int ret;
    /* Write 1876916 as unsigned LEB128. */
    uint8_t uval[] = {0xB4, 0xC7, 0x72};
    for (size_t i = 0; i < sizeof(uval); i++) {
	ret = fwrite(&uval[i], 1, 1, f);
	if (ret < 1) {
	    fprintf(stderr, "fwrite writing less than expected\n");
	    fclose(f);
	    return -1;
	}
    }

    /* Write 0 as unsigned LEB128. */
    uint8_t zero_uval = 0;
    ret = fwrite(&zero_uval, 1, 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Write -220236 as signed LEB128 */
    for (size_t i = 0; i < sizeof(uval); i++) {
	ret = fwrite(&uval[i], 1, 1, f);
	if (ret < 1) {
	    fprintf(stderr, "fwrite writing less than expected\n");
	    fclose(f);
	    return -1;
	}
    }

    /* Write 0 as signed LEB128. */
    ret = fwrite(&zero_uval, 1, 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Write 1876916 as signed LEB128. */
    uint8_t uval2[] = {0xB4, 0xC7, 0xF2, 0x0};
    for (size_t i = 0; i < sizeof(uval2); i++) {
	ret = fwrite(&uval2[i], 1, 1, f);
	if (ret < 1) {
	    fprintf(stderr, "fwrite writing less than expected\n");
	    fclose(f);
	    return -1;
	}
    }

    /* Write INT64_MIN as signed LEB128, -9223372036854775808 */
    uint8_t int64_min[] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7f};
    for (size_t i = 0; i < sizeof(int64_min); i++) {
	ret = fwrite(&int64_min[i], 1, 1, f);
	if (ret < 1) {
	    fprintf(stderr, "fwrite writing less than expected\n");
	    fclose(f);
	    return -1;
	}
    }

    fclose(f);

    return 0;
}
