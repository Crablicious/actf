#include <endian.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>


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


static struct metadata_pkt_hdr BASE_LE_HDR = {
    .magic = 0x75d11d57,
    .uuid = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
    .checksum = 1337,
    // .content_sz_bits = 352,
    // .total_sz_bits = 352,
    .major = 2,
    .hdr_sz_bits = 352,
};


int main(int argc, char *argv[])
{
    if (argc < 3) {
	fprintf(stderr, "Usage: ./metadata_pkt_hdr_pack be/le INPATH OUTPATH\n");
	return -1;
    }

    bool do_be = false;
    if (strcmp(argv[1], "be") == 0) {
	do_be = true;
    } else if (strcmp(argv[1], "le") == 0) {
	do_be = false;
    } else {
	fprintf(stderr, "first argument should be byte order (be/le)");
	return -1;
    }

    FILE *f_in = fopen(argv[2], "r");
    if (!f_in) {
	perror("fopen");
	return -1;
    }
    FILE *f_out = fopen(argv[3], "w");
    if (!f_out) {
	perror("fopen");
	fclose(f_in);
	return -1;
    }

    struct segment {
	int sz; // bytes
	int padding; // bytes
    };

    struct segment segments[] = {
	{.sz = 22, .padding = 12},
	{.sz = 24, .padding = 0},
	{.sz = 0, .padding = 24},
	{.sz = 140, .padding = 1},
	{.sz = 0, .padding = 0},
	{.sz = -1, .padding = 200}, // remainder
    };
    int rc = 0;
    char b[1024]; // yolo
    struct metadata_pkt_hdr hdr = BASE_LE_HDR;
    if (do_be) {
	hdr.magic = htobe32(hdr.magic);
	hdr.hdr_sz_bits = htobe32(hdr.hdr_sz_bits);
    }
    for (size_t i = 0; i < sizeof(segments)/sizeof(*segments); i++) {
	memset(b, 0, 1024);
	struct segment seg = segments[i];
	size_t sz = seg.sz >= 0 ? seg.sz : 1024;
	rc = fread(b, 1, sz, f_in);
	if (rc < 0) {
	    perror("fread");
	    goto out;
	}
	sz = rc;
	hdr.content_sz_bits = sizeof(hdr) * 8 + sz * 8;
	hdr.total_sz_bits = hdr.content_sz_bits + seg.padding * 8;
	if (do_be) {
	    hdr.content_sz_bits = htobe32(hdr.content_sz_bits);
	    hdr.total_sz_bits = htobe32(hdr.total_sz_bits);
	}

	rc = fwrite(&hdr, sizeof(hdr), 1, f_out);
	if (rc < 0) {
	    perror("fwrite");
	    goto out;
	}
	rc = fwrite(b, 1, sz + seg.padding, f_out);
	if (rc < 0) {
	    perror("fwrite");
	    goto out;
	}
    }

    rc = 0;
out:
    fclose(f_out);
    fclose(f_in);

    return rc;
}
