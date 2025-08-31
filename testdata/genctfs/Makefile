CC	= clang
CFLAGS	= -g -Wall
LDFLAGS	=

.PHONY: all
all: bit_dump pkt_hdr_dump pkt_ctxt_dump pkt_ctxt_align_eof_content_dump \
	ev_rec_hdr_dump variant_dump \
	optional_dump static_str_dump dyn_str_dump var_len_int_dump \
	fxd_len_float_dump static_len_arr_fld_loc_dump dyn_len_arr_fld_loc_dump \
	fld_loc_double_arr_dump metadata_pkt_hdr_dump metadata_pkt_hdr_pack

bit_dump.o:
pkt_hdr_dump.o:
pkt_ctxt_dump.o:
pkt_ctxt_align_eof_content_dump.o:
ev_rec_hdr_dump.o:
variant_dump.o:
optional_dump.o:
static_str_dump.o:
dyn_str_dump.o:
var_len_int_dump.o:
fxd_len_float_dump.o:
static_len_arr_fld_loc_dump.o:
dyn_len_arr_fld_loc_dump.o:
fld_loc_double_arr_dump.o:
metadata_pkt_hdr_dump.o:
metadata_pkt_hdr_pack.o:

bit_dump: bit_dump.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

pkt_hdr_dump: pkt_hdr_dump.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

pkt_ctxt_dump: pkt_ctxt_dump.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

pkt_ctxt_align_eof_content_dump: pkt_ctxt_align_eof_content_dump.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

ev_rec_hdr_dump: ev_rec_hdr_dump.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

variant_dump: variant_dump.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

optional_dump: optional_dump.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

static_str_dump: static_str_dump.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

dyn_str_dump: dyn_str_dump.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

var_len_int_dump: var_len_int_dump.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

fxd_len_float_dump: fxd_len_float_dump.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

static_len_arr_fld_loc_dump: static_len_arr_fld_loc_dump.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

dyn_len_arr_fld_loc_dump: dyn_len_arr_fld_loc_dump.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

fld_loc_double_arr_dump: fld_loc_double_arr_dump.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

metadata_pkt_hdr_dump: metadata_pkt_hdr_dump.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

metadata_pkt_hdr_pack: metadata_pkt_hdr_pack.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	$(RM) *.o bit_dump pkt_hdr_dump pkt_ctxt_dump pkt_ctxt_align_eof_content_dump \
	ev_rec_hdr_dump variant_dump \
	optional_dump static_str_dump dyn_str_dump var_len_int_dump fxd_len_float_dump \
	static_len_arr_fld_loc_dump dyn_len_arr_fld_loc_dump fld_loc_double_arr_dump \
	metadata_pkt_hdr_dump metadata_pkt_hdr_pack
