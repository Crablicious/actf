/*
 * This file is a part of ACTF.
 *
 * Copyright (C) 2024  Adam Wendelin <adwe live se>
 *
 * ACTF is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * ACTF is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General
 * Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ACTF. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <CUnit/CUnit.h>
#include <CUnit/TestDB.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "metadata_int.h"
#include "test_metadata.h"


static struct actf_metadata *metadata;

static const char *minimal_preamble =
    "{"
    "    \"type\": \"preamble\","
    "    \"version\": 2"
    "}";

static const char *uuid_preamble =
    "{"
    "    \"type\": \"preamble\","
    "    \"version\": 2,"
    "    \"uuid\": ["
    "        229, 62, 10, 184, 80, 161, 79, 10,"
    "        183, 16, 181, 240, 187, 169, 196, 172"
    "    ]"
    "}";

static const char *illegal_preamble =
    "{"
    "    \"type\": \"preamble\","
    "    \"version\": 1"
    "}";

/* Preamble with declared extensions (illegal) */
static const char *extensions_preamble =
    "{"
    "    \"type\": \"preamble\","
    "    \"version\": 2,"
    "    \"extensions\": {"
    "        \"my.namespace\": {"
    "            \"decl ext 1\": {"
    "                \"max-bit-len\": 20,"
    "            },"
    "            \"disable-ints\": null"
    "        }"
    "    }"
    "}";

/* Preamble without declared extensions (legal) */
static const char *extensions_no_declared_preamble =
    "{"
    "    \"type\": \"preamble\","
    "    \"version\": 2,"
    "    \"extensions\": {"
    "    }"
    "}";

static const char *attributes_preamble =
    "{"
    "    \"type\": \"preamble\","
    "    \"version\": 2,"
    "    \"attributes\": {"
    "        \"my.tracer\": {"
    "            \"max-count\": 45,"
    "            \"module\": \"sys\""
    "        },"
    "        \"abc/xyz\": true"
    "    }"
    "}";

static const char *minimal_fld_cls_alias =
    "{"
    "    \"type\": \"field-class-alias\","
    "    \"name\": \"u8\","
    "    \"field-class\": {"
    "        \"type\": \"fixed-length-unsigned-integer\","
    "        \"length\": 8,"
    "        \"byte-order\": \"little-endian\""
    "    }"
    "}";

static const char *fld_cls_alias_to_alias =
    "{"
    "    \"type\": \"field-class-alias\","
    "    \"name\": \"uint8\","
    "    \"field-class\": \"u8\""
    "}";

static const char *pkt_hdr_alias =
    "{"
    "    \"type\": \"field-class-alias\","
    "    \"name\": \"packet header alias\","
    "    \"field-class\": {"
    "        \"type\": \"structure\","
    "        \"member-classes\": ["
    "        {"
    "            \"name\": \"the magic!\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-unsigned-integer\","
    "                \"length\": 32,"
    "                \"byte-order\": \"little-endian\","
    "                \"preferred-display-base\": 16,"
    "                \"roles\": [\"packet-magic-number\"]"
    "            }"
    "        },"
    "        {"
    "            \"name\": \"my data stream class ID\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-unsigned-integer\","
    "                \"length\": 8,"
    "                \"byte-order\": \"little-endian\","
    "                \"roles\": [\"data-stream-class-id\"]"
    "            }"
    "        }"
    "        ]"
    "    }"
    "}";

static const char *illegal_pkt_hdr_alias =
    "{"
    "    \"type\": \"field-class-alias\","
    "    \"name\": \"packet header alias\","
    "    \"field-class\": {"
    "        \"type\": \"fixed-length-unsigned-integer\","
    "        \"length\": 8,"
    "        \"byte-order\": \"little-endian\""
    "    }"
    "}";

static const char *minimal_trace_cls =
    "{"
    "    \"type\": \"trace-class\","
    "}";

static const char *alias_pkt_hdr_trace_cls =
    "{"
    "    \"type\": \"trace-class\","
    "    \"uid\": \"the uid\","
    "    \"packet-header-field-class\": \"packet header alias\""
    "}";

static const char *roles_trace_cls =
    "{"
    "    \"type\": \"trace-class\","
    "    \"uid\": \"the uid\","
    "    \"packet-header-field-class\": {"
    "        \"type\": \"structure\","
    "        \"member-classes\": ["
    "            {"
    "            \"name\": \"the magic!\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-unsigned-integer\","
    "                \"length\": 32,"
    "                \"byte-order\": \"little-endian\","
    "                \"preferred-display-base\": 16,"
    "                \"roles\": [\"packet-magic-number\"]"
    "            }"
    "        },"
    "        {"
    "            \"name\": \"my data stream class ID\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-unsigned-integer\","
    "                \"length\": 8,"
    "                \"byte-order\": \"little-endian\","
    "                \"roles\": [\"data-stream-class-id\"]"
    "            }"
    "        },"
    "        {"
    "            \"name\": \"my metadata stream uuid\","
    "            \"field-class\": {"
    "                \"type\": \"static-length-blob\","
    "                \"length\": 16,"
    "                \"roles\": [\"metadata-stream-uuid\"]"
    "            }"
    "        },"
    "        ]"
    "    }"
    "}";

static const char *illegal_pkt_hdr_role_trace_cls =
    "{"
    "    \"type\": \"trace-class\","
    "    \"uid\": \"the uid\","
    "    \"packet-header-field-class\": {"
    "        \"type\": \"structure\","
    "        \"member-classes\": ["
    "        {"
    "            \"name\": \"my data stream class ID\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-unsigned-integer\","
    "                \"length\": 8,"
    "                \"byte-order\": \"little-endian\","
    "                \"roles\": [\"data-stream-class-id\"]"
    "            }"
    "        },"
    "        {"
    "            \"name\": \"the magic!\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-unsigned-integer\","
    "                \"length\": 32,"
    "                \"byte-order\": \"little-endian\","
    "                \"preferred-display-base\": 16,"
    "                \"roles\": [\"packet-magic-number\"]"
    "            }"
    "        },"
    "        ]"
    "    }"
    "}";

static const char *illegal_pkt_hdr_role_trace_cls2 =
    "{"
    "    \"type\": \"trace-class\","
    "    \"uid\": \"the uid\","
    "    \"packet-header-field-class\": {"
    "        \"type\": \"structure\","
    "        \"member-classes\": ["
    "        {"
    "            \"name\": \"illegal pkt hdr\","
    "            \"field-class\": {"
    "                \"type\": \"structure\","
    "                \"member-classes\": ["
    "                {"
    "                    \"name\": \"the magic!\","
    "                    \"field-class\": {"
    "                        \"type\": \"fixed-length-unsigned-integer\","
    "                        \"length\": 32,"
    "                        \"byte-order\": \"little-endian\","
    "                        \"preferred-display-base\": 16,"
    "                        \"roles\": [\"packet-magic-number\"]"
    "                    }"
    "                }"
    "                ]"
    "            }"
    "        }"
    "        ]"
    "    }"
    "}";

static const char *illegal_stream_uuid_trace_cls =
    "{"
    "    \"type\": \"trace-class\","
    "    \"uid\": \"the uid\","
    "    \"packet-header-field-class\": {"
    "        \"type\": \"structure\","
    "        \"member-classes\": ["
    "        {"
    "            \"name\": \"my metadata stream uuid\","
    "            \"field-class\": {"
    "                \"type\": \"static-length-blob\","
    "                \"length\": 18,"  // Wrong length
    "                \"roles\": [\"metadata-stream-uuid\"]"
    "            }"
    "        }"
    "        ]"
    "    }"
    "}";

static const char *full_trace_cls =
    "{"
    "    \"type\": \"trace-class\","
    "    \"uid\": \"the uid\","
    "    \"packet-header-field-class\": {"
    "        \"type\": \"structure\","
    "        \"member-classes\": ["
    "            {"
    "            \"name\": \"the magic!\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-unsigned-integer\","
    "                \"length\": 32,"
    "                \"byte-order\": \"little-endian\","
    "                \"preferred-display-base\": 16,"
    "                \"roles\": [\"packet-magic-number\"]"
    "            }"
    "        },"
    "            {"
    "            \"name\": \"the UUID\","
    "            \"field-class\": {"
    "                \"type\": \"static-length-blob\","
    "                \"length\": 16,"
    "                \"roles\": [\"metadata-stream-uuid\"]"
    "            }"
    "        },"
    "            {"
    "            \"name\": \"my data stream class ID\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-unsigned-integer\","
    "                \"length\": 8,"
    "                \"byte-order\": \"little-endian\","
    "                \"roles\": [\"data-stream-class-id\"]"
    "            }"
    "        },"
    "            {"
    "            \"name\": \"my data stream ID\","
    "            \"field-class\": {"
    "                \"type\": \"variable-length-unsigned-integer\","
    "                \"roles\": [\"data-stream-id\"]"
    "            }"
    "        }"
    "        ]"
    "    }"
    "}";

static const char *environment_trace_cls =
    "{"
    "    \"type\": \"trace-class\","
    "    \"environment\": {"
    "        \"roflmao\": 1337,"
    "        \"xdd\": \"oh god no\""
    "    }"
    "}";

static const char *invalid_environment_trace_cls =
    "{"
    "    \"type\": \"trace-class\","
    "    \"environment\": {"
    "        \"my.tracer\": {"
    "            \"max-count\": 45,"
    "            \"module\": \"sys\""
    "        },"
    "        \"abc/xyz\": true"
    "    }"
    "}";

static const char *attributes_trace_cls =
    "{"
    "    \"type\": \"trace-class\","
    "    \"attributes\": {"
    "        \"my.tracer\": {"
    "            \"max-count\": 45,"
    "            \"module\": \"sys\""
    "        },"
    "        \"abc/xyz\": true"
    "    }"
    "}";

static const char *extensions_trace_cls =
    "{"
    "    \"type\": \"trace-class\","
    "    \"extensions\": {"
    "        \"my.tracer\": {"
    "            \"keys\": 88,"
    "            \"temperament\": \"equal\""
    "        }"
    "    }"
    "}";

static const char *minimal_clk_cls =
    "{"
    "    \"type\": \"clock-class\","
    "    \"id\": \"my clock class\","
    "    \"frequency\": 1000000000"
    "}";

static const char *unix_origin_clk_cls =
    "{"
    "    \"type\": \"clock-class\","
    "    \"id\": \"my unix origin clock class\","
    "    \"frequency\": 1000000000,"
    "    \"origin\": \"unix-epoch\""
    "}";

static const char *custom_origin_clk_cls =
    "{"
    "    \"type\": \"clock-class\","
    "    \"id\": \"my custom origin clock class\","
    "    \"frequency\": 1000000000,"
    "    \"origin\": {"
    "        \"name\": \"my origin\","
    "        \"uid\": \"60:57:18:a3:42:29\""
    "    }"
    "}";

static const char *off_clk_cls =
    "{"
    "    \"type\": \"clock-class\","
    "    \"id\": \"my offset clock class\","
    "    \"frequency\": 1000000000,"
    "    \"offset-from-origin\": {"
    "        \"seconds\": 1605112699,"
    "        \"cycles\": 2878388"
    "    }"
    "}";

static const char *precision_clk_cls =
    "{"
    "    \"type\": \"clock-class\","
    "    \"id\": \"my precision clock class\","
    "    \"frequency\": 1000000000,"
    "    \"precision\": 100,"
    "    \"accuracy\": 50"
    "}";

/* cycles larger than frequency */
static const char *illegal_off_clk_cls =
    "{"
    "    \"type\": \"clock-class\","
    "    \"id\": \"my illegal offset clock class\","
    "    \"frequency\": 500000,"
    "    \"offset-from-origin\": {"
    "        \"seconds\": 1605112699,"
    "        \"cycles\": 2878388"
    "    }"
    "}";

static const char *illegal_off_clk_cls2 =
    "{"
    "    \"type\": \"clock-class\","
    "    \"id\": \"my illegal offset clock class\","
    "    \"frequency\": 2878388,"
    "    \"offset-from-origin\": {"
    "        \"seconds\": 1605112699,"
    "        \"cycles\": 2878388"
    "    }"
    "}";

static const char *illegal_freq_clk_cls =
    "{"
    "    \"type\": \"clock-class\","
    "    \"id\": \"my illegal freq clock class\","
    "    \"frequency\": -500000,"
    "}";

static const char *illegal_accuracy_clk_cls =
    "{"
    "    \"type\": \"clock-class\","
    "    \"id\": \"my illegal accuracy clock class\","
    "    \"accuracy\": -1,"
    "}";

static const char *minimal_ds_cls =
    "{"
    "    \"type\": \"data-stream-class\""
    "}";

static const char *minimal_ds_cls_id1 =
    "{"
    "    \"type\": \"data-stream-class\","
    "    \"id\": 1"
    "}";

static const char *def_clk_ds_cls =
    "{"
    "    \"type\": \"data-stream-class\","
    "    \"default-clock-class-id\": \"my unix origin clock class\""
    "}";

static const char *illegal_def_clk_ds_cls =
    "{"
    "    \"type\": \"data-stream-class\","
    "    \"default-clock-class-id\": \"non-existing clock name\""
    "}";

static const char *pkt_ctx_ds_cls =
    "{"
    "    \"type\": \"data-stream-class\","
    "    \"name\": \"beautiful name\","
    "    \"packet-context-field-class\": {"
    "        \"type\": \"structure\","
    "        \"member-classes\": ["
    "        {"
    "            \"name\": \"simple 32 bits little endian\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-bit-array\","
    "                \"length\": 32,"
    "                \"byte-order\": \"little-endian\","
    "            }"
    "        },"
    "        {"
    "            \"name\": \"simple 16 bits big endian\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-bit-array\","
    "                \"length\": 8,"
    "                \"byte-order\": \"big-endian\","
    "            }"
    "        },"
    "        ]"
    "    }"
    "}";

static const char *event_hdr_ds_cls =
    "{"
    "    \"type\": \"data-stream-class\","
    "    \"name\": \"beautiful name\","
    "    \"default-clock-class-id\": \"my unix origin clock class\","
    "    \"event-record-header-field-class\": {"
    "        \"type\": \"structure\","
    "        \"member-classes\": ["
    "        {"
    "            \"name\": \"simple 32 bits little endian\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-unsigned-integer\","
    "                \"length\": 32,"
    "                \"byte-order\": \"little-endian\","
    "                \"roles\": [\"default-clock-timestamp\"]"
    "            }"
    "        },"
    "        {"
    "            \"name\": \"simple 16 bits big endian\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-unsigned-integer\","
    "                \"length\": 8,"
    "                \"byte-order\": \"big-endian\","
    "                \"roles\": [\"event-record-class-id\"]"
    "            }"
    "        },"
    "        ]"
    "    }"
    "}";

static const char *event_common_ctx_ds_cls =
    "{"
    "    \"type\": \"data-stream-class\","
    "    \"name\": \"beautiful name\","
    "    \"event-record-common-context-field-class\": {"
    "        \"type\": \"structure\","
    "        \"member-classes\": ["
    "        {"
    "            \"name\": \"simple 32 bits little endian\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-unsigned-integer\","
    "                \"length\": 32,"
    "                \"byte-order\": \"little-endian\""
    "            }"
    "        },"
    "        {"
    "            \"name\": \"simple 16 bits big endian\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-unsigned-integer\","
    "                \"length\": 8,"
    "                \"byte-order\": \"big-endian\""
    "            }"
    "        },"
    "        ]"
    "    }"
    "}";

static const char *attributes_ds_cls =
    "{"
    "    \"type\": \"data-stream-class\","
    "    \"attributes\": {"
    "        \"my.tracer\": {"
    "            \"max-count\": 45,"
    "            \"module\": \"sys\""
    "        },"
    "        \"abc/xyz\": true"
    "    }"
    "}";

static const char *invalid_attributes_event_hdr_ds_cls =
    "{"
    "    \"type\": \"data-stream-class\","
    "    \"name\": \"beautiful name\","
    "    \"default-clock-class-id\": \"my unix origin clock class\","
    "    \"event-record-header-field-class\": {"
    "        \"type\": \"structure\","
    "        \"member-classes\": ["
    "        {"
    "            \"name\": \"simple 32 bits little endian\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-unsigned-integer\","
    "                \"length\": 32,"
    "                \"byte-order\": \"little-endian\","
    "                \"roles\": [\"default-clock-timestamp\"]"
    "            }"
    "        },"
    "        {"
    "            \"name\": \"simple 16 bits big endian\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-unsigned-integer\","
    "                \"length\": 8,"
    "                \"byte-order\": \"big-endian\","
    "                \"roles\": [\"event-record-class-id\"]"
    "            }"
    "        },"
    "        ]"
    "    },"
    "    \"attributes\": \"illegal attribute, not an object!\""
    "}";

static const char *minimal_event_cls =
    "{"
    "    \"type\": \"event-record-class\""
    "}";

static const char *payload_id1_event_cls =
    "{"
    "    \"type\": \"event-record-class\","
    "    \"id\": 1,"
    "    \"name\": \"named id1 event record\","
    "    \"payload-field-class\": {"
    "        \"type\": \"structure\","
    "        \"member-classes\": ["
    "        {"
    "            \"name\": \"simple 64 bits little endian\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-bit-array\","
    "                \"length\": 64,"
    "                \"byte-order\": \"little-endian\","
    "            }"
    "        }"
    "        ]"
    "    }"
    "}";

static const char *spec_ctx_event_cls =
    "{"
    "    \"type\": \"event-record-class\","
    "    \"name\": \"named id0 event record\","
    "    \"specific-context-field-class\": {"
    "        \"type\": \"structure\","
    "        \"member-classes\": ["
    "        {"
    "            \"name\": \"simple 64 bits little endian\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-bit-array\","
    "                \"length\": 64,"
    "                \"byte-order\": \"little-endian\","
    "            }"
    "        }"
    "        ]"
    "    }"
    "}";

static const char *variant_event_cls =
    "{"
    "    \"type\": \"event-record-class\","
    "    \"name\": \"named id0 event record\","
    "    \"specific-context-field-class\": {"
    "        \"type\": \"structure\","
    "        \"member-classes\": ["
    "        {"
    "            \"name\": \"8 bit selector\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-unsigned-integer\","
    "                \"length\": 8,"
    "                \"byte-order\": \"little-endian\","
    "            }"
    "        }"
    "        ]"
    "    },"
    "    \"payload-field-class\": {"
    "        \"type\": \"structure\","
    "        \"member-classes\": ["
    "        {"
    "            \"name\": \"8 bit selector\","
    "            \"field-class\": {"
    "                \"type\": \"variant\","
    "                \"selector-field-location\":{"
    "                  \"origin\": \"event-record-specific-context\","
    "                  \"path\": [\"8 bit selector\"]"
    "                },"
    "                \"options\": ["
    "                  {"
    "                    \"selector-field-ranges\": [[5, 5]],"
    "                    \"field-class\": {"
    "                      \"type\": \"fixed-length-signed-integer\","
    "                      \"length\": 8,"
    "                      \"byte-order\": \"little-endian\""
    "                    }"
    "                  },"
    "                  {"
    "                    \"selector-field-ranges\": [[8, 8]],"
    "                    \"field-class\": {"
    "                      \"type\": \"fixed-length-unsigned-integer\","
    "                      \"length\": 16,"
    "                      \"byte-order\": \"little-endian\""
    "                    }"
    "                  }"
    "                ]"
    "            }"
    "        }"
    "        ]"
    "    }"
    "}";

/* Illegal because variant options have overlapping ranges. */
static const char *illegal_variant_event_cls =
    "{"
    "    \"type\": \"event-record-class\","
    "    \"name\": \"named id0 event record\","
    "    \"specific-context-field-class\": {"
    "        \"type\": \"structure\","
    "        \"member-classes\": ["
    "        {"
    "            \"name\": \"8 bit selector\","
    "            \"field-class\": {"
    "                \"type\": \"fixed-length-unsigned-integer\","
    "                \"length\": 8,"
    "                \"byte-order\": \"little-endian\","
    "            }"
    "        }"
    "        ]"
    "    },"
    "    \"payload-field-class\": {"
    "        \"type\": \"structure\","
    "        \"member-classes\": ["
    "        {"
    "            \"name\": \"8 bit selector\","
    "            \"field-class\": {"
    "                \"type\": \"variant\","
    "                \"selector-field-location\":{"
    "                  \"origin\": \"event-record-specific-context\","
    "                  \"path\": [\"8 bit selector\"]"
    "                },"
    "                \"options\": ["
    "                  {"
    "                    \"selector-field-ranges\": [[5, 10]],"
    "                    \"field-class\": {"
    "                      \"type\": \"fixed-length-signed-integer\","
    "                      \"length\": 8,"
    "                      \"byte-order\": \"little-endian\""
    "                    }"
    "                  },"
    "                  {"
    "                    \"selector-field-ranges\": [[8, 8]],"
    "                    \"field-class\": {"
    "                      \"type\": \"fixed-length-unsigned-integer\","
    "                      \"length\": 16,"
    "                      \"byte-order\": \"little-endian\""
    "                    }"
    "                  }"
    "                ]"
    "            }"
    "        }"
    "        ]"
    "    }"
    "}";

static const char *extensions_event_cls =
    "{"
    "    \"type\": \"event-record-class\","
    "    \"extensions\": {"
    "        \"my.tracer\": {"
    "            \"keys\": 88,"
    "            \"temperament\": \"equal\""
    "        }"
    "    }"
    "}";


static int test_metadata_suite_init(void)
{
    return 0;
}

static int test_metadata_suite_clean(void)
{
    return 0;
}

static void test_metadata_test_setup(void)
{
    return;
}

static void test_metadata_test_teardown(void)
{
    return;
}

static void test_metadata_preamble(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) != 0);
    actf_metadata_free(metadata);

    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, uuid_preamble) == 0);
    actf_metadata_free(metadata);

    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, illegal_preamble) != 0);
    actf_metadata_free(metadata);

    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, extensions_preamble) != 0);
    actf_metadata_free(metadata);

    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, extensions_no_declared_preamble) == 0);
    actf_metadata_free(metadata);
}

static void test_metadata_preamble_attributes(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT_PTR_NULL(actf_preamble_attributes(&metadata->preamble));
    actf_metadata_free(metadata);

    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT_FATAL(actf_metadata_parse(metadata, attributes_preamble) == 0);
    const struct actf_fld *attrs = actf_preamble_attributes(&metadata->preamble);
    CU_ASSERT_PTR_NOT_NULL_FATAL(attrs);
    const struct actf_fld *abcxyz = actf_fld_struct_fld(attrs, "abc/xyz");
    CU_ASSERT_PTR_NOT_NULL_FATAL(abcxyz);
    CU_ASSERT_TRUE(actf_fld_bool(abcxyz));
    actf_metadata_free(metadata);
}

static void test_metadata_fld_cls_alias(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_fld_cls_alias) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, fld_cls_alias_to_alias) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_fld_cls_alias) != 0);
    actf_metadata_free(metadata);
}

static void test_metadata_trace_cls(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_trace_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_trace_cls) != 0);
    actf_metadata_free(metadata);
}

static void test_metadata_trace_cls_full(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, uuid_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, full_trace_cls) == 0);
    actf_metadata_free(metadata);
}

static void test_metadata_trace_cls_roles(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, uuid_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, roles_trace_cls) == 0);
    actf_metadata_free(metadata);

    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, illegal_pkt_hdr_role_trace_cls) != 0);
    CU_ASSERT(actf_metadata_parse(metadata, illegal_pkt_hdr_role_trace_cls2) != 0);
    actf_metadata_free(metadata);

    /* No preamble with a metadata-stream-uuid */
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, roles_trace_cls) != 0);
    actf_metadata_free(metadata);

    /* Illegal field classes holding metadata-stream-uuid */
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, uuid_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, illegal_stream_uuid_trace_cls) != 0);
    actf_metadata_free(metadata);
}

static void test_metadata_trace_cls_pkt_hdr_alias(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    /* No alias added, should fail */
    CU_ASSERT(actf_metadata_parse(metadata, alias_pkt_hdr_trace_cls) != 0);
    /* Add alias */
    CU_ASSERT(actf_metadata_parse(metadata, pkt_hdr_alias) == 0);
    /* Adding should now work */
    CU_ASSERT(actf_metadata_parse(metadata, alias_pkt_hdr_trace_cls) == 0);
    actf_metadata_free(metadata);

    /* Try adding a trace class with a packet context referring to an
     * alias that is not a structure.
     */
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, illegal_pkt_hdr_alias) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, alias_pkt_hdr_trace_cls) != 0);
    actf_metadata_free(metadata);
}

static void test_metadata_trace_cls_environment(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, environment_trace_cls) == 0);
    const struct actf_fld *env = actf_trace_cls_environment(&metadata->trace_cls);
    CU_ASSERT_PTR_NOT_NULL_FATAL(env);
    CU_ASSERT(actf_fld_struct_len(env) == 2);
    const struct actf_fld *roflmao = actf_fld_struct_fld(env, "roflmao");
    CU_ASSERT_EQUAL(actf_fld_uint64(roflmao), 1337);
    const struct actf_fld *xdd = actf_fld_struct_fld(env, "xdd");
    CU_ASSERT_STRING_EQUAL(actf_fld_str_raw(xdd), "oh god no");
    actf_metadata_free(metadata);

    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, invalid_environment_trace_cls) != 0);
    actf_metadata_free(metadata);
}

static void test_metadata_trace_cls_attributes(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, attributes_trace_cls) == 0);
    const struct actf_fld *attrs = actf_trace_cls_attributes(&metadata->trace_cls);
    CU_ASSERT_PTR_NOT_NULL_FATAL(attrs);
    CU_ASSERT(actf_fld_struct_len(attrs) == 2);
    actf_metadata_free(metadata);
}

static void test_metadata_trace_cls_extensions(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, extensions_trace_cls) == 0);
    const struct actf_fld *exts = actf_trace_cls_extensions(&metadata->trace_cls);
    CU_ASSERT_PTR_NOT_NULL_FATAL(exts);
    CU_ASSERT(actf_fld_struct_len(exts) == 1);
    actf_metadata_free(metadata);
}

static void test_metadata_clk_cls(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_clk_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, unix_origin_clk_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, custom_origin_clk_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, off_clk_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, precision_clk_cls) == 0);

    /* Adding a clock class with identical name as an already existing
     * clock is not allowed
     */
    CU_ASSERT(actf_metadata_parse(metadata, unix_origin_clk_cls) != 0);

    /* Having a cycle offset that is greater or equal to the frequency
     * is not allowed.
     */
    CU_ASSERT(actf_metadata_parse(metadata, illegal_off_clk_cls) != 0);
    CU_ASSERT(actf_metadata_parse(metadata, illegal_off_clk_cls2) != 0);

    /* Having a negative frequency is not allowed. */
    CU_ASSERT(actf_metadata_parse(metadata, illegal_freq_clk_cls) != 0);

    /* Having a negative accuracy is not allowed. */
    CU_ASSERT(actf_metadata_parse(metadata, illegal_accuracy_clk_cls) != 0);

    actf_metadata_free(metadata);
}

static void test_metadata_ds_cls(void)
{
    /* Test minimal ds cls */
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_ds_cls) == 0);
    actf_metadata_free(metadata);

    /* Test a ds cls with a packet context */
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, pkt_ctx_ds_cls) == 0);
    actf_metadata_free(metadata);

    /* Test multiple ds clses with different IDs */
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_ds_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_ds_cls_id1) == 0);
    actf_metadata_free(metadata);

    /* Test multiple ds clses with same ID */
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_ds_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_ds_cls) != 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_ds_cls_id1) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_ds_cls_id1) != 0);
    actf_metadata_free(metadata);
}

static void test_metadata_ds_cls_with_def_clk(void)
{
    /* Test happy and unhappy adding of a ds cls with a clk cls. */
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_clk_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, unix_origin_clk_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, custom_origin_clk_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, def_clk_ds_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, illegal_def_clk_ds_cls) != 0);
    actf_metadata_free(metadata);

    /* Make sure nothing funny happens even with no clk clses
     * specified.
     */
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, illegal_def_clk_ds_cls) != 0);
    actf_metadata_free(metadata);
}

static void test_metadata_ds_cls_with_event_hdr(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, unix_origin_clk_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, event_hdr_ds_cls) == 0);
    actf_metadata_free(metadata);
}

static void test_metadata_ds_cls_with_event_common_ctx(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, event_common_ctx_ds_cls) == 0);
    actf_metadata_free(metadata);
}

static void test_metadata_ds_cls_attributes(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, attributes_ds_cls) == 0);
    struct actf_it it = {0};
    const struct actf_fld *attrs = actf_dstream_cls_attributes(actf_metadata_dstream_clses_next(metadata, &it));
    CU_ASSERT_PTR_NOT_NULL_FATAL(attrs);
    CU_ASSERT(actf_fld_struct_len(attrs) == 2);
    actf_metadata_free(metadata);

    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, invalid_attributes_event_hdr_ds_cls) != 0);
    actf_metadata_free(metadata);
}

static void test_metadata_event_cls(void)
{
    /* Test minimal ev record cls */
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_ds_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_event_cls) == 0);
    actf_metadata_free(metadata);

    /* Test adding ev record without a ds cls */
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_event_cls) != 0);
    actf_metadata_free(metadata);

    /* Test adding ev record with a ds cls but not matching id */
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_ds_cls_id1) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_event_cls) != 0);
    actf_metadata_free(metadata);

    /* Test multiple ev record */
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_ds_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_event_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, payload_id1_event_cls) == 0);
    actf_metadata_free(metadata);

    /* Test multiple ev record with same ID */
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_ds_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, payload_id1_event_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, payload_id1_event_cls) != 0);
    actf_metadata_free(metadata);
}

static void test_metadata_event_cls_with_spec_ctx(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_ds_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, spec_ctx_event_cls) == 0);
    actf_metadata_free(metadata);
}

static void test_metadata_event_cls_with_variant(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_ds_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, variant_event_cls) == 0);
    actf_metadata_free(metadata);

    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_ds_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, illegal_variant_event_cls) != 0);
    actf_metadata_free(metadata);
}

static void test_metadata_event_cls_with_extensions(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL((metadata = actf_metadata_init()));
    CU_ASSERT(actf_metadata_parse(metadata, minimal_preamble) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, minimal_ds_cls) == 0);
    CU_ASSERT(actf_metadata_parse(metadata, extensions_event_cls) == 0);
    struct actf_it dscit = {0};
    struct actf_it evcit = {0};
    const struct actf_event_cls *evc = actf_dstream_cls_event_clses_next(
	actf_metadata_dstream_clses_next(metadata, &dscit), &evcit);
    CU_ASSERT_PTR_NOT_NULL_FATAL(evc);
    const struct actf_fld *exts = actf_event_cls_extensions(evc);
    CU_ASSERT_PTR_NOT_NULL_FATAL(exts);
    CU_ASSERT(actf_fld_struct_len(exts) == 1);
    actf_metadata_free(metadata);
}

static void test_metadata_nparse_packetized(void)
{
    const char *path = "testdata/ctfs/CTF2-PMETA-1.0-be/metadata";
    int rc;
    struct stat sb;
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
	CU_FAIL_FATAL("open");
    }
    rc = fstat(fd, &sb);
    if (rc < 0) {
	close(fd);
	CU_FAIL_FATAL("fstat");
    }
    char *b = malloc(sb.st_size);
    if (! b) {
	close(fd);
	CU_FAIL_FATAL("malloc");
    }
    ssize_t n_read = read(fd, b, sb.st_size);
    CU_ASSERT_FATAL(n_read == sb.st_size);

    metadata = actf_metadata_init();
    if (! metadata) {
	CU_FAIL("actf_metadata_init");
	goto out;
    }
    rc = actf_metadata_nparse(metadata, b, sb.st_size);
    if (rc < 0) {
	CU_FAIL("actf_metadata_nparse");
	goto out;
    }

out:
    free(b);
    close(fd);
}

static void test_clk_cls_eq_identities(void)
{
    struct actf_clk_cls clkc1 = {
	.id = "monotonic", .namespace = NULL, .name = "linux", .uid = NULL,
	.origin = {.type = ACTF_CLK_ORIGIN_TYPE_UNIX_EPOCH}, .has_precision = true,
	.precision = 200};
    struct actf_clk_cls clkc2 = {
	.id = "monotonic", .namespace = NULL, .name = "linux", .uid = NULL,
	.origin = {.type = ACTF_CLK_ORIGIN_TYPE_UNIX_EPOCH}, .has_precision = false};

    CU_ASSERT_TRUE(actf_clk_cls_eq_identities(&clkc1, &clkc2));
    CU_ASSERT_FALSE(actf_clk_cls_eq_identities_strict(&clkc1, &clkc2));

    clkc2.has_precision = true;
    clkc2.precision = 200;
    CU_ASSERT_TRUE(actf_clk_cls_eq_identities_strict(&clkc1, &clkc2));

    clkc1.origin = (struct actf_clk_origin){.type = ACTF_CLK_ORIGIN_TYPE_CUSTOM,
	.custom = {.namespace = NULL, .name = "raspberry", .uid = "1234"}};
    clkc2.origin = (struct actf_clk_origin){.type = ACTF_CLK_ORIGIN_TYPE_CUSTOM,
	.custom = {.namespace = "uh oh", .name = "raspberry", .uid = "1234"}};
    CU_ASSERT_TRUE(actf_clk_cls_eq_identities(&clkc1, &clkc2));
    CU_ASSERT_FALSE(actf_clk_cls_eq_identities_strict(&clkc1, &clkc2));

    clkc1.origin.custom.namespace = "uh oh";
    CU_ASSERT_TRUE(actf_clk_cls_eq_identities_strict(&clkc1, &clkc2));
}

static CU_TestInfo test_metadata_tests[] = {
    {"preamble", test_metadata_preamble},
    {"preamble attributes", test_metadata_preamble_attributes},
    {"fld cls alias", test_metadata_fld_cls_alias},
    {"trace cls", test_metadata_trace_cls},
    {"trace cls roles", test_metadata_trace_cls_roles},
    {"trace cls pkt hdr alias", test_metadata_trace_cls_pkt_hdr_alias},
    {"trace cls full", test_metadata_trace_cls_full},
    {"trace cls environment", test_metadata_trace_cls_environment},
    {"trace cls attributes", test_metadata_trace_cls_attributes},
    {"trace cls extensions", test_metadata_trace_cls_extensions},
    {"clk cls", test_metadata_clk_cls},
    {"ds cls", test_metadata_ds_cls},
    {"ds cls with def clk", test_metadata_ds_cls_with_def_clk},
    {"ds cls with event record header", test_metadata_ds_cls_with_event_hdr},
    {"ds cls with event record common ctx", test_metadata_ds_cls_with_event_common_ctx},
    {"ds cls with attributes", test_metadata_ds_cls_attributes},
    {"ev record cls", test_metadata_event_cls},
    {"ev record cls with specific context", test_metadata_event_cls_with_spec_ctx},
    {"ev record cls with variant", test_metadata_event_cls_with_variant},
    {"ev record cls with extensions", test_metadata_event_cls_with_extensions},
    {"nparse with packetized metadata", test_metadata_nparse_packetized},
    {"clk cls eq identities", test_clk_cls_eq_identities},
    CU_TEST_INFO_NULL,
};

CU_SuiteInfo test_metadata_suite = {
    "Metadata", test_metadata_suite_init, test_metadata_suite_clean,
    test_metadata_test_setup, test_metadata_test_teardown, test_metadata_tests
};
