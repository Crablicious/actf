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

#ifndef ACTF_FLD_INT_H
#define ACTF_FLD_INT_H

#include "fld.h"

/* Try to keep this struct at 64-bytes or below (maybe pad to 64?) */
struct actf_fld {
	enum actf_fld_type type;
	union {
		struct {
			bool val;
		} bool_;
		struct {
			uint64_t val;
			// length in bits if value belongs to a
			// variable-length-unsigned-integer class. Required to
			// handle the edge-case where a clock-value is encoded
			// using a var-int and padded to be longer than required.
			size_t len;
		} uint;
		struct {
			int64_t val;
		} int_;
		struct {
			uint64_t val;
		} bit_map;
		union {
			float f32;
			double f64;
		} real;
		struct {
			struct actf_fld *vals;
			size_t n_vals;
		} arr;
		struct {
			// TODO: Assumes that it is fine to directly refer to the
			// buffer we read from which means the read-buffer's
			// lifetime can't be shorter than the actf_flds. With mmap
			// it's 100% fine.
			const uint8_t *ptr;
			// len specifies the valid amount of bytes in ptr. For
			// string-classes, len includes the null-terminator (if it
			// exists). A static length string with a null-terminator
			// is defined to have garbage bytes after the
			// null-terminator, those are not included in len. To see
			// the on-disk size of a static length string, you must
			// use the class.
			size_t len;
		} str;
		struct {
			// see comment in str about lifetime.
			const uint8_t *ptr;
			// len specifies the valid amount of bytes in ptr.
			size_t len;
		} blob;
		struct {
			/* We rely on the field class to know the number of `vals`
			 * and the (name + fld_cls) of each val.
			 */
			struct actf_fld *vals;
		} struct_;
	} d;
	/* The class the `actf_fld` belongs to. Needed for at least
	 * mappings and preferred-display-base. Might be useful as well to
	 * encode actf_fld.
	 */
	const struct actf_fld_cls *cls;

	/* The struct holding the actf_fld. Mainly relevant to lookup a
	 * field location with a relative origin.
	 */
	struct actf_fld *parent;
};

#endif /* ACTF_FLD_INT_H */
