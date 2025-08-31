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

    /* Packet magic num */
    val32 = 0xc1fc1fc1;
    ret = fwrite(&val32, sizeof(val32), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Metadata-stream-uuid */
    uint8_t uuid[] = {
	229, 62, 10, 184, 80, 161, 79, 10,
	183, 16, 181, 240, 187, 169, 196, 172
    };
    for (size_t i = 0; i < sizeof(uuid); i++) {
	val8 = uuid[i];
	ret = fwrite(&val8, sizeof(val8), 1, f);
	if (ret < 1) {
	    fprintf(stderr, "fwrite writing less than expected\n");
	    fclose(f);
	    return -1;
	}
    }

    /* Data stream class id */
    val8 = 13;
    ret = fwrite(&val8, sizeof(val8), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    /* Data stream id (not used afaik) */
    val8 = 4;
    ret = fwrite(&val8, sizeof(val8), 1, f);
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

    fclose(f);

    return 0;
}
