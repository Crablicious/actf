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
    uint64_t val64;
    size_t ret;

    /* 208 bits pkt header
       32 bits event record
    */

/*****************************************************************************/
/*                                  Packet 1                                 */
/*****************************************************************************/

    /* Packet begin tstamp */
    val64 = 0x1;
    ret = fwrite(&val64, sizeof(val64), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Discarded event counter */
    val8 = 3;
    ret = fwrite(&val8, sizeof(val8), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Packet content length */
    val32 = 272;
    ret = fwrite(&val32, sizeof(val32), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Packet end tstamp */
    val64 = 0x1000;
    ret = fwrite(&val64, sizeof(val64), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Packet seq number */
    val8 = 8;
    ret = fwrite(&val8, sizeof(val8), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Packet total length */
    val32 = 304;
    ret = fwrite(&val32, sizeof(val32), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Event record 0 */
    val32 = 0xdeadbeef;
    ret = fwrite(&val32, sizeof(val32), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Event record 1 */
    val32 = 0xcafebabe;
    ret = fwrite(&val32, sizeof(val32), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Padding */
    val32 = 0xFFFFFFFF;
    ret = fwrite(&val32, sizeof(val32), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

/*****************************************************************************/
/*                                  Packet 2                                 */
/*****************************************************************************/

    /* Packet begin tstamp */
    val64 = 0x1001;
    ret = fwrite(&val64, sizeof(val64), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Discarded event counter */
    val8 = 255;
    ret = fwrite(&val8, sizeof(val8), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Packet content length */
    val32 = 272;
    ret = fwrite(&val32, sizeof(val32), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Packet end tstamp */
    val64 = 0x2000;
    ret = fwrite(&val64, sizeof(val64), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Packet seq number */
    val8 = 9;
    ret = fwrite(&val8, sizeof(val8), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Packet total length */
    val32 = 272;
    ret = fwrite(&val32, sizeof(val32), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Event record 0 */
    val32 = 0xfeedbabe;
    ret = fwrite(&val32, sizeof(val32), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Event record 1 */
    val32 = 0x1337beef;
    ret = fwrite(&val32, sizeof(val32), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Garbage data after packet */
    val32 = 0xFFFFFFFF;
    ret = fwrite(&val32, sizeof(val32), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    fclose(f);

    return 0;
}
