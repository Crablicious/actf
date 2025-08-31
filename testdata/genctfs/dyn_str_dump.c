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
    const char teststr[] = "You Can Fly! You Can Fly! You Can Fly!";
    uint32_t val32;
    uint32_t len = sizeof(teststr);

    /* Length */
    val32 = len;
    ret = fwrite(&val32, sizeof(val32), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Teststr */
    ret = fwrite(teststr, 1, len, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    val32 = 0;
    ret = fwrite(&val32, sizeof(val32), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    fclose(f);

    return 0;
}
