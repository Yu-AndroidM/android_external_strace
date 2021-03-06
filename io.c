/*
 * Copyright (c) 1991, 1992 Paul Kranenburg <pk@cs.few.eur.nl>
 * Copyright (c) 1993 Branko Lankester <branko@hacktic.nl>
 * Copyright (c) 1993, 1994, 1995, 1996 Rick Sladkey <jrs@world.std.com>
 * Copyright (c) 1996-1999 Wichert Akkerman <wichert@cistron.nl>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "defs.h"
#include <fcntl.h>
#include <sys/uio.h>

SYS_FUNC(read)
{
	if (entering(tcp)) {
		printfd(tcp, tcp->u_arg[0]);
		tprints(", ");
	} else {
		if (syserror(tcp))
			tprintf("%#lx", tcp->u_arg[1]);
		else
			printstr(tcp, tcp->u_arg[1], tcp->u_rval);
		tprintf(", %lu", tcp->u_arg[2]);
	}
	return 0;
}

SYS_FUNC(write)
{
	if (entering(tcp)) {
		printfd(tcp, tcp->u_arg[0]);
		tprints(", ");
		printstr(tcp, tcp->u_arg[1], tcp->u_arg[2]);
		tprintf(", %lu", tcp->u_arg[2]);
	}
	return 0;
}

/*
 * data_size limits the cumulative size of printed data.
 * Example: recvmsg returing a short read.
 */
void
tprint_iov_upto(struct tcb *tcp, unsigned long len, unsigned long addr, int decode_iov, unsigned long data_size)
{
#if SUPPORTED_PERSONALITIES > 1
	union {
		struct { u_int32_t base; u_int32_t len; } iov32;
		struct { u_int64_t base; u_int64_t len; } iov64;
	} iov;
#define sizeof_iov \
	(current_wordsize == 4 ? sizeof(iov.iov32) : sizeof(iov.iov64))
#define iov_iov_base \
	(current_wordsize == 4 ? (uint64_t) iov.iov32.base : iov.iov64.base)
#define iov_iov_len \
	(current_wordsize == 4 ? (uint64_t) iov.iov32.len : iov.iov64.len)
#else
	struct iovec iov;
#define sizeof_iov sizeof(iov)
#define iov_iov_base iov.iov_base
#define iov_iov_len iov.iov_len
#endif
	unsigned long size, cur, end, abbrev_end;
	int failed = 0;

	if (!len) {
		tprints("[]");
		return;
	}
	size = len * sizeof_iov;
	end = addr + size;
	if (!verbose(tcp) || size / sizeof_iov != len || end < addr) {
		tprintf("%#lx", addr);
		return;
	}
	if (abbrev(tcp)) {
		abbrev_end = addr + max_strlen * sizeof_iov;
		if (abbrev_end < addr)
			abbrev_end = end;
	} else {
		abbrev_end = end;
	}
	tprints("[");
	for (cur = addr; cur < end; cur += sizeof_iov) {
		if (cur > addr)
			tprints(", ");
		if (cur >= abbrev_end) {
			tprints("...");
			break;
		}
		if (umoven(tcp, cur, sizeof_iov, &iov) < 0) {
			tprints("?");
			failed = 1;
			break;
		}
		tprints("{");
		if (decode_iov) {
			unsigned long len = iov_iov_len;
			if (len > data_size)
				len = data_size;
			data_size -= len;
			printstr(tcp, (long) iov_iov_base, len);
		} else
			tprintf("%#lx", (long) iov_iov_base);
		tprintf(", %lu}", (unsigned long)iov_iov_len);
	}
	tprints("]");
	if (failed)
		tprintf(" %#lx", addr);
#undef sizeof_iov
#undef iov_iov_base
#undef iov_iov_len
}

void
tprint_iov(struct tcb *tcp, unsigned long len, unsigned long addr, int decode_iov)
{
	tprint_iov_upto(tcp, len, addr, decode_iov, (unsigned long) -1L);
}

SYS_FUNC(readv)
{
	if (entering(tcp)) {
		printfd(tcp, tcp->u_arg[0]);
		tprints(", ");
	} else {
		if (syserror(tcp)) {
			tprintf("%#lx, %lu",
					tcp->u_arg[1], tcp->u_arg[2]);
			return 0;
		}
		tprint_iov(tcp, tcp->u_arg[2], tcp->u_arg[1], 1);
		tprintf(", %lu", tcp->u_arg[2]);
	}
	return 0;
}

SYS_FUNC(writev)
{
	if (entering(tcp)) {
		printfd(tcp, tcp->u_arg[0]);
		tprints(", ");
		tprint_iov(tcp, tcp->u_arg[2], tcp->u_arg[1], 1);
		tprintf(", %lu", tcp->u_arg[2]);
	}
	return 0;
}

/* The SH4 ABI does allow long longs in odd-numbered registers, but
   does not allow them to be split between registers and memory - and
   there are only four argument registers for normal functions.  As a
   result pread takes an extra padding argument before the offset.  This
   was changed late in the 2.4 series (around 2.4.20).  */
#if defined(SH)
#define PREAD_OFFSET_ARG 4
#else
#define PREAD_OFFSET_ARG 3
#endif

SYS_FUNC(pread)
{
	if (entering(tcp)) {
		printfd(tcp, tcp->u_arg[0]);
		tprints(", ");
	} else {
		if (syserror(tcp))
			tprintf("%#lx", tcp->u_arg[1]);
		else
			printstr(tcp, tcp->u_arg[1], tcp->u_rval);
		tprintf(", %lu, ", tcp->u_arg[2]);
		printllval(tcp, "%llu", PREAD_OFFSET_ARG);
	}
	return 0;
}

SYS_FUNC(pwrite)
{
	if (entering(tcp)) {
		printfd(tcp, tcp->u_arg[0]);
		tprints(", ");
		printstr(tcp, tcp->u_arg[1], tcp->u_arg[2]);
		tprintf(", %lu, ", tcp->u_arg[2]);
		printllval(tcp, "%llu", PREAD_OFFSET_ARG);
	}
	return 0;
}

static void
print_llu_from_low_high_val(struct tcb *tcp, int arg)
{
#if SIZEOF_LONG == SIZEOF_LONG_LONG
# if SUPPORTED_PERSONALITIES > 1
	if (current_wordsize == sizeof(long))
# endif
		tprintf("%lu", (unsigned long) tcp->u_arg[arg]);
# if SUPPORTED_PERSONALITIES > 1
	else
		tprintf("%lu",
			((unsigned long) tcp->u_arg[arg + 1] << current_wordsize * 8)
			| (unsigned long) tcp->u_arg[arg]);
# endif
#else
# ifdef X32
	if (current_personality == 0)
		tprintf("%llu", (unsigned long long) tcp->ext_arg[arg]);
	else
# endif
	tprintf("%llu",
		((unsigned long long) (unsigned long) tcp->u_arg[arg + 1] << sizeof(long) * 8)
		| (unsigned long long) (unsigned long) tcp->u_arg[arg]);
#endif
}

SYS_FUNC(preadv)
{
	if (entering(tcp)) {
		printfd(tcp, tcp->u_arg[0]);
		tprints(", ");
	} else {
		if (syserror(tcp)) {
			tprintf("%#lx, %lu", tcp->u_arg[1], tcp->u_arg[2]);
			return 0;
		}
		tprint_iov(tcp, tcp->u_arg[2], tcp->u_arg[1], 1);
		tprintf(", %lu, ", tcp->u_arg[2]);
		print_llu_from_low_high_val(tcp, 3);
	}
	return 0;
}

SYS_FUNC(pwritev)
{
	if (entering(tcp)) {
		printfd(tcp, tcp->u_arg[0]);
		tprints(", ");
		tprint_iov(tcp, tcp->u_arg[2], tcp->u_arg[1], 1);
		tprintf(", %lu, ", tcp->u_arg[2]);
		print_llu_from_low_high_val(tcp, 3);
	}
	return 0;
}

static void
print_off_t(struct tcb *tcp, long addr)
{
	unsigned long offset;

	if (!addr) {
		tprints("NULL");
		return;
	}

#if SUPPORTED_PERSONALITIES > 1 && SIZEOF_LONG > 4
	if (current_wordsize == 4) {
		uint32_t off;

		if (umove(tcp, addr, &off) < 0)
			tprintf("%#lx", addr);
		else
			tprintf("[%u]", off);
	} else
#endif
	if (umove(tcp, addr, &offset) < 0)
		tprintf("%#lx", addr);
	else
		tprintf("[%lu]", offset);
}

SYS_FUNC(sendfile)
{
	if (entering(tcp)) {
		printfd(tcp, tcp->u_arg[0]);
		tprints(", ");
		printfd(tcp, tcp->u_arg[1]);
		tprints(", ");
		print_off_t(tcp, tcp->u_arg[2]);
		tprintf(", %lu", tcp->u_arg[3]);
	}
	return 0;
}

void
print_loff_t(struct tcb *tcp, long addr)
{
	loff_t offset;

	if (!addr)
		tprints("NULL");
	else if (umove(tcp, addr, &offset) < 0)
		tprintf("%#lx", addr);
	else
		tprintf("[%llu]", (unsigned long long int) offset);
}

SYS_FUNC(sendfile64)
{
	if (entering(tcp)) {
		printfd(tcp, tcp->u_arg[0]);
		tprints(", ");
		printfd(tcp, tcp->u_arg[1]);
		tprints(", ");
		print_loff_t(tcp, tcp->u_arg[2]);
		tprintf(", %lu", tcp->u_arg[3]);
	}
	return 0;
}

#include "xlat/splice_flags.h"

SYS_FUNC(tee)
{
	if (entering(tcp)) {
		/* int fd_in */
		printfd(tcp, tcp->u_arg[0]);
		tprints(", ");
		/* int fd_out */
		printfd(tcp, tcp->u_arg[1]);
		tprints(", ");
		/* size_t len */
		tprintf("%lu, ", tcp->u_arg[2]);
		/* unsigned int flags */
		printflags(splice_flags, tcp->u_arg[3], "SPLICE_F_???");
	}
	return 0;
}

SYS_FUNC(splice)
{
	if (entering(tcp)) {
		/* int fd_in */
		printfd(tcp, tcp->u_arg[0]);
		tprints(", ");
		/* loff_t *off_in */
		print_loff_t(tcp, tcp->u_arg[1]);
		tprints(", ");
		/* int fd_out */
		printfd(tcp, tcp->u_arg[2]);
		tprints(", ");
		/* loff_t *off_out */
		print_loff_t(tcp, tcp->u_arg[3]);
		tprints(", ");
		/* size_t len */
		tprintf("%lu, ", tcp->u_arg[4]);
		/* unsigned int flags */
		printflags(splice_flags, tcp->u_arg[5], "SPLICE_F_???");
	}
	return 0;
}

SYS_FUNC(vmsplice)
{
	if (entering(tcp)) {
		/* int fd */
		printfd(tcp, tcp->u_arg[0]);
		tprints(", ");
		/* const struct iovec *iov, unsigned long nr_segs */
		tprint_iov(tcp, tcp->u_arg[2], tcp->u_arg[1], 1);
		tprintf(", %lu, ", tcp->u_arg[2]);
		/* unsigned int flags */
		printflags(splice_flags, tcp->u_arg[3], "SPLICE_F_???");
	}
	return 0;
}

SYS_FUNC(ioctl)
{
	const struct_ioctlent *iop;

	if (entering(tcp)) {
		printfd(tcp, tcp->u_arg[0]);
		tprints(", ");
		if (!ioctl_decode_command_number(tcp->u_arg[1])) {
			iop = ioctl_lookup(tcp->u_arg[1]);
			if (iop) {
				tprints(iop->symbol);
				while ((iop = ioctl_next_match(iop)))
					tprintf(" or %s", iop->symbol);
			} else {
				ioctl_print_code(tcp->u_arg[1]);
			}
		}
		ioctl_decode(tcp, tcp->u_arg[1], tcp->u_arg[2]);
	}
	else {
		int ret = ioctl_decode(tcp, tcp->u_arg[1], tcp->u_arg[2]);
		if (!ret)
			tprintf(", %#lx", tcp->u_arg[2]);
		else
			return ret - 1;
	}
	return 0;
}
