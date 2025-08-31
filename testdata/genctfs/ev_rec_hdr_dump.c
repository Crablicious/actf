#include <endian.h>
#include <stdio.h>
#include <stdint.h>


int main(int argc, char *argv[])
{
    if (argc < 2) {
	fprintf(stderr, "Usage: ./pkt_hdr_dump DS_PATH\n");
	return -1;
    }
    FILE *f = fopen(argv[1], "w");
    if (!f) {
	perror("fopen");
	return -1;
    }

    uint8_t val8;
    uint32_t val32;
    size_t ret;

/*****************************************************************************/
/*                                  Event 0                                  */
/*****************************************************************************/

    /* Current timestamp */
    val32 = 304;
    ret = fwrite(&val32, sizeof(val32), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }
    /* Event record class id */
    val8 = 8;
    ret = fwrite(&val8, sizeof(val8), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }
    /* Event record 0 */
    val8 = 133;
    ret = fwrite(&val8, sizeof(val8), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

/*****************************************************************************/
/*                                  Event 1                                  */
/*****************************************************************************/

    /* Current timestamp */
    val32 = 310;
    ret = fwrite(&val32, sizeof(val32), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }
    /* Event record class id */
    val8 = 5;
    ret = fwrite(&val8, sizeof(val8), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }
    /* Event record 1 */
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
