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

    /* _Float16 f16 = 12.34; */
    /* ret = fwrite(&f16, sizeof(f16), 1, f); */
    /* if (ret < 1) { */
    /* 	fprintf(stderr, "fwrite writing less than expected\n"); */
    /* 	fclose(f); */
    /* 	return -1; */
    /* } */

    _Float32 f32 = 1234.5678;
    ret = fwrite(&f32, sizeof(f32), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    _Float64 f64 = 1234.5678910;
    ret = fwrite(&f64, sizeof(f64), 1, f);
    if (ret < 1) {
	fprintf(stderr, "fwrite writing less than expected\n");
	fclose(f);
	return -1;
    }

    fclose(f);

    return 0;
}
