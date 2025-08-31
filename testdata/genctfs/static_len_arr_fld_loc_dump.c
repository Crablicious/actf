#include <endian.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>


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

    const char *teststrs[] = {
	"Raccoons are one of the few animals that are able to use all five fingers like humans do.",
	"As a result, they can successfully complete many unusual functions",
	"like opening the latch of a cage, untying knots,",
	"and even writing Python code.",
    };

    // my struct array
    for (int i = 0; i < 4; i++) {
	int ret;
	const char *teststr = teststrs[i];
	uint32_t val32;
	uint32_t len = strlen(teststr) + 1;

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
    }

    fclose(f);

    return 0;
}
