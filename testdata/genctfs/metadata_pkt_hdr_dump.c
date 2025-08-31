#include <endian.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>


struct metadata_pkt_hdr {
    /* Must match `METADATA_PKT_HDR_MAGIC`. The byte-order of this
     * field decides the byte-order of the rest of the header. */
    uint32_t magic;
    /* UUID for all metadata packets wrapping a metadata stream */
    uint8_t uuid[16];
    /* Must ignore */
    uint32_t checksum;
    /* Size of metadata packet including this header. Must be multiple of
     * 8. */
    uint32_t content_sz_bits;
    /* Total size of the whole metadata packet including this header
     * and padding. Must be a multiple of 8. */
    uint32_t total_sz_bits;
    /* Must be zero (no compression) */
    uint8_t compression_scheme;
    /* Must be zero (no encryption) */
    uint8_t encryption_scheme;
    /* Must be zero (no checksum) */
    uint8_t content_checksum;
    /* Must be set to 2 */
    uint8_t major;
    /* Must be set to 0 */
    uint8_t minor;
    uint8_t reserved[3];
    /* Size of whole header. Must be 352 (44 bytes). */
    uint32_t hdr_sz_bits;
} __attribute__((__packed__));


int main(int argc, char *argv[])
{
    if (argc < 2) {
	fprintf(stderr, "Usage: ./metadata_pkt_hdr_dump PATH\n");
	return -1;
    }
    FILE *f = fopen(argv[1], "w");
    if (!f) {
	perror("fopen");
	return -1;
    }

    struct metadata_pkt_hdr hdr = {
	.magic = 0x75d11d57,
	.uuid = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
	.checksum = 1337,
	.content_sz_bits = 352, // SET THIS BASED ON CONTENT!
	.total_sz_bits = 352,  // SET THIS BASED ON CONTENT!
	.major = 2,
	.hdr_sz_bits = 352,
    };

    int ret;
    ret = fwrite(&hdr, sizeof(hdr), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    fclose(f);

    return 0;
}
