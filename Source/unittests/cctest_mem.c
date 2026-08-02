/*
 * cctest_mem.c - memory LVOs
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include "cctest_util.h"

void
ct_suite_mem(void)
{
	char a[32];
	char b[32];
	char c[32];
	char src[16];
	APTR p;
	LONG i;
	LONG r;
	UBYTE ub;

	ct_suite_begin((STRPTR)"mem", (STRPTR)"memset memcpy memcmp bcmp bcopy bzero memmove memchr memccpy memswap");

	/* --- memcpy basic --- */
	memset(a, 0, sizeof(a));
	memcpy(a, (APTR)"hi", 3);
	ct_check_string((STRPTR)"mem",
		(STRPTR)"memcpy_with_nul",
		(STRPTR)"memcpy(a, \"hi\", 3)",
		(STRPTR)"memcpy must copy three bytes including the NUL terminator of \"hi\"",
		(STRPTR)"hi",
		a);

	memset(a, 0, sizeof(a));
	memcpy(a, (APTR)"ABCD", 4);
	ct_check_true((STRPTR)"mem", (STRPTR)"memcpy_four_bytes",
		(STRPTR)"memcmp after memcpy(a,\"ABCD\",4)",
		(STRPTR)"memcpy of four bytes must reproduce the source exactly",
		memcmp((APTR)a, (APTR)"ABCD", 4) == 0);

	memset(a, 0, sizeof(a));
	memcpy(a, (APTR)"x", 1);
	ct_check_true((STRPTR)"mem", (STRPTR)"memcpy_one_byte",
		(STRPTR)"memcpy(a, \"x\", 1)",
		(STRPTR)"memcpy of one byte must copy that byte without requiring a NUL",
		(UBYTE)a[0] == (UBYTE)'x');

	/* --- memcpy / memcmp roundtrip --- */
	memset(a, 0, sizeof(a));
	memset(b, 0, sizeof(b));
	memcpy(a, (APTR)"roundtrip", 10);
	memcpy(b, a, 10);
	ct_check_long((STRPTR)"mem", (STRPTR)"memcpy_memcmp_roundtrip",
		(STRPTR)"memcmp(a, b, 10) after memcpy roundtrip",
		(STRPTR)"memcpy into a second buffer must yield memcmp-equal contents",
		0, memcmp((APTR)a, (APTR)b, 10));

	memset(a, 0, sizeof(a));
	memcpy(a, (APTR)"0123456789", 10);
	memcpy(b, a, 5);
	ct_check_true((STRPTR)"mem", (STRPTR)"memcpy_partial_roundtrip",
		(STRPTR)"memcmp(b, \"01234\", 5) after partial memcpy",
		(STRPTR)"partial memcpy must copy only the requested byte count",
		memcmp((APTR)b, (APTR)"01234", 5) == 0);

	/* --- memset --- */
	memset(a, 'X', 8);
	a[8] = '\0';
	ct_check_string((STRPTR)"mem",
		(STRPTR)"memset_fill_char",
		(STRPTR)"memset(a, 'X', 8)",
		(STRPTR)"memset of 8 bytes to 'X' must produce eight X characters",
		(STRPTR)"XXXXXXXX",
		a);

	memset(a, 0, 16);
	ct_check_true((STRPTR)"mem", (STRPTR)"memset_zero",
		(STRPTR)"memset(a, 0, 16)",
		(STRPTR)"memset with value 0 must clear every byte in the range",
		a[0] == 0 && a[7] == 0 && a[15] == 0);

	for (i = 0; i < 16; i++)
		a[i] = (char)0xA5;
	memset(a, 0x11, 7);
	ct_check_true((STRPTR)"mem", (STRPTR)"memset_length_boundary",
		(STRPTR)"memset(a, 0x11, 7) on 0xA5 buffer",
		(STRPTR)"memset must alter exactly n bytes and leave the byte at index n unchanged",
		(UBYTE)a[0] == 0x11 && (UBYTE)a[6] == 0x11 && (UBYTE)a[7] == 0xA5);

	memset(a, 'A', 1);
	ct_check_true((STRPTR)"mem", (STRPTR)"memset_one_byte",
		(STRPTR)"memset(a, 'A', 1)",
		(STRPTR)"memset of length 1 must set only the first byte",
		a[0] == 'A');

	memset(a, 0xFF, 4);
	ct_check_true((STRPTR)"mem", (STRPTR)"memset_0xFF",
		(STRPTR)"memset(a, 0xFF, 4)",
		(STRPTR)"memset must accept any unsigned byte value as fill",
		(UBYTE)a[0] == 0xFF && (UBYTE)a[3] == 0xFF);

	/* --- memset then memchr --- */
	memset(a, 0, sizeof(a));
	memset(a, 'Z', 6);
	p = memchr((APTR)a, (ULONG)'Z', 6);
	ct_check_true((STRPTR)"mem", (STRPTR)"memset_memchr_found",
		(STRPTR)"memchr(a, 'Z', 6) after memset",
		(STRPTR)"memchr must locate a byte set by memset within the given length",
		p != (APTR)NULL && *((char *)p) == 'Z');

	memset(a, 'A', 6);
	p = memchr((APTR)a, (ULONG)'B', 6);
	ct_check_ptr_null((STRPTR)"mem", (STRPTR)"memset_memchr_missing",
		(STRPTR)"memchr(a, 'B', 6) after memset to 'A'",
		(STRPTR)"memchr must return NULL when the byte is absent in the range",
		p);

	/* --- memcmp --- */
	ct_check_long((STRPTR)"mem", (STRPTR)"memcmp_equal",
		(STRPTR)"memcmp(\"abcd\", \"abcd\", 4)",
		(STRPTR)"memcmp of two identical regions must return 0",
		0, memcmp((APTR)"abcd", (APTR)"abcd", 4));

	ct_check_true((STRPTR)"mem", (STRPTR)"memcmp_diff",
		(STRPTR)"memcmp(\"abcd\", \"abxd\", 4) != 0",
		(STRPTR)"memcmp must return non-zero when bytes differ within the length",
		memcmp((APTR)"abcd", (APTR)"abxd", 4) != 0);

	ct_check_true((STRPTR)"mem", (STRPTR)"memcmp_less",
		(STRPTR)"memcmp(\"a\", \"b\", 1) < 0",
		(STRPTR)"memcmp must return negative when the first region sorts before the second",
		memcmp((APTR)"a", (APTR)"b", 1) < 0);

	ct_check_true((STRPTR)"mem", (STRPTR)"memcmp_greater",
		(STRPTR)"memcmp(\"b\", \"a\", 1) > 0",
		(STRPTR)"memcmp must return positive when the first region sorts after the second",
		memcmp((APTR)"b", (APTR)"a", 1) > 0);

	ct_check_long((STRPTR)"mem", (STRPTR)"memcmp_partial_equal",
		(STRPTR)"memcmp(\"abcX\", \"abcY\", 3)",
		(STRPTR)"memcmp with length shorter than differing suffix must return 0",
		0, memcmp((APTR)"abcX", (APTR)"abcY", 3));

	ct_check_true((STRPTR)"mem", (STRPTR)"memcmp_partial_diff",
		(STRPTR)"memcmp(\"abcX\", \"abcY\", 4) != 0",
		(STRPTR)"memcmp must detect a difference at the final compared byte",
		memcmp((APTR)"abcX", (APTR)"abcY", 4) != 0);

	ct_check_long((STRPTR)"mem", (STRPTR)"memcmp_n_zero",
		(STRPTR)"memcmp(\"abc\", \"xyz\", 0)",
		(STRPTR)"memcmp with length 0 must compare no bytes and return 0",
		0, memcmp((APTR)"abc", (APTR)"xyz", 0));

	/* --- bcmp (BSD name; same ordered compare as memcmp) --- */
	ct_check_long((STRPTR)"mem", (STRPTR)"bcmp_equal",
		(STRPTR)"bcmp(\"zz\", \"zz\", 2)",
		(STRPTR)"bcmp is the BSD alias of memcmp; equal buffers must return 0",
		0, bcmp((APTR)"zz", (APTR)"zz", 2));

	ct_check_true((STRPTR)"mem", (STRPTR)"bcmp_diff",
		(STRPTR)"bcmp(\"aa\", \"ab\", 2) != 0",
		(STRPTR)"bcmp must detect differing bytes like memcmp",
		bcmp((APTR)"aa", (APTR)"ab", 2) != 0);

	ct_check_true((STRPTR)"mem", (STRPTR)"bcmp_matches_memcmp",
		(STRPTR)"bcmp(\"data\", \"data\", 4) == memcmp(\"data\", \"data\", 4)",
		(STRPTR)"bcmp and memcmp must agree on equal four-byte regions",
		bcmp((APTR)"data", (APTR)"data", 4) == memcmp((APTR)"data", (APTR)"data", 4));

	/* --- bcopy (BSD arg order: src, dst) --- */
	strcpy(a, (STRPTR)"12345678");
	bcopy((APTR)"ABCD", a, 4);
	a[4] = '\0';
	ct_check_string((STRPTR)"mem",
		(STRPTR)"bcopy_overwrite_prefix",
		(STRPTR)"bcopy(\"ABCD\", a, 4) over \"12345678\"",
		(STRPTR)"bcopy must overwrite the first n bytes of the destination",
		(STRPTR)"ABCD",
		a);

	strcpy(a, (STRPTR)"xxxxxxxx");
	bcopy((APTR)"Hi", a, 2);
	ct_check_true((STRPTR)"mem", (STRPTR)"bcopy_two_bytes",
		(STRPTR)"bcopy(\"Hi\", a, 2)",
		(STRPTR)"bcopy of two bytes must set both destination bytes",
		a[0] == 'H' && a[1] == 'i');

	memset(b, 0, sizeof(b));
	bcopy((APTR)"test", b, 4);
	ct_check_true((STRPTR)"mem", (STRPTR)"bcopy_to_clean_buffer",
		(STRPTR)"memcmp(b, \"test\", 4) after bcopy",
		(STRPTR)"bcopy into a zeroed buffer must reproduce the source bytes",
		memcmp((APTR)b, (APTR)"test", 4) == 0);

	/* --- memmove (C arg order: dst, src; overlap-safe) --- */
	strcpy(a, (STRPTR)"12345678");
	memmove(a, (APTR)"ABCD", 4);
	a[4] = '\0';
	ct_check_string((STRPTR)"mem",
		(STRPTR)"memmove_overwrite_prefix",
		(STRPTR)"memmove(a, \"ABCD\", 4) over \"12345678\"",
		(STRPTR)"memmove must overwrite the first n bytes of the destination",
		(STRPTR)"ABCD",
		a);

	strcpy(a, (STRPTR)"xxxxxxxx");
	memmove(a, (APTR)"Hi", 2);
	ct_check_true((STRPTR)"mem", (STRPTR)"memmove_two_bytes",
		(STRPTR)"memmove(a, \"Hi\", 2)",
		(STRPTR)"memmove of two bytes must set both destination bytes",
		a[0] == 'H' && a[1] == 'i');

	memset(b, 0, sizeof(b));
	memmove(b, (APTR)"test", 4);
	ct_check_true((STRPTR)"mem", (STRPTR)"memmove_to_clean_buffer",
		(STRPTR)"memcmp(b, \"test\", 4) after memmove",
		(STRPTR)"memmove into a zeroed buffer must reproduce the source bytes",
		memcmp((APTR)b, (APTR)"test", 4) == 0);

	/* Overlapping region: shift right by one within the same buffer. */
	strcpy(a, (STRPTR)"abcdef");
	memmove(a + 1, a, 5);
	a[6] = '\0';
	ct_check_string((STRPTR)"mem",
		(STRPTR)"memmove_overlap_right",
		(STRPTR)"memmove(a+1, a, 5) on \"abcdef\"",
		(STRPTR)"memmove must copy safely when source and destination overlap",
		(STRPTR)"aabcde",
		a);

	/* --- bzero (BSD zero-fill LVO) --- */
	strcpy(a, (STRPTR)"dirty!!");
	bzero(a, 5);
	ct_check_true((STRPTR)"mem", (STRPTR)"bzero_partial",
		(STRPTR)"bzero(a, 5) on \"dirty!!\"",
		(STRPTR)"bzero must clear exactly n bytes and leave later bytes untouched",
		a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0 && a[4] == 0
		&& a[5] == '!' && a[6] == '!');

	memset(a, 0xFF, 8);
	bzero(a, 8);
	ct_check_true((STRPTR)"mem", (STRPTR)"bzero_full_range",
		(STRPTR)"bzero(a, 8) on 0xFF buffer",
		(STRPTR)"bzero must clear every byte in the requested range",
		a[0] == 0 && a[7] == 0);

	memset(a, 0x55, 4);
	bzero(a, 1);
	ct_check_true((STRPTR)"mem", (STRPTR)"bzero_one_byte",
		(STRPTR)"bzero(a, 1)",
		(STRPTR)"bzero of length 1 must clear only the first byte",
		a[0] == 0 && a[1] == 0x55);

	/* --- memset as zero-fill (same effect as bzero when c==0) --- */
	strcpy(a, (STRPTR)"dirty!!");
	memset(a, 0, 5);
	ct_check_true((STRPTR)"mem", (STRPTR)"memset_partial_zero",
		(STRPTR)"memset(a, 0, 5) on \"dirty!!\"",
		(STRPTR)"memset must clear exactly n bytes and leave later bytes untouched",
		a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0 && a[4] == 0
		&& a[5] == '!' && a[6] == '!');

	memset(a, 0xFF, 8);
	memset(a, 0, 8);
	ct_check_true((STRPTR)"mem", (STRPTR)"memset_full_zero",
		(STRPTR)"memset(a, 0, 8) on 0xFF buffer",
		(STRPTR)"memset must clear every byte in the requested range",
		a[0] == 0 && a[7] == 0);

	memset(a, 0, 1);
	ct_check_true((STRPTR)"mem", (STRPTR)"memset_one_byte_zero",
		(STRPTR)"memset(a, 0, 1)",
		(STRPTR)"memset of length 1 must clear only the first byte",
		a[0] == 0);

	/* --- memchr --- */
	p = memchr((APTR)"abcdef", (ULONG)'d', 6);
	ct_check_true((STRPTR)"mem", (STRPTR)"memchr_found_middle",
		(STRPTR)"memchr(\"abcdef\", 'd', 6)",
		(STRPTR)"memchr must return a pointer to the first matching byte",
		p != (APTR)NULL && *((char *)p) == 'd');

	p = memchr((APTR)"abcdef", (ULONG)'a', 6);
	ct_check_true((STRPTR)"mem", (STRPTR)"memchr_found_first",
		(STRPTR)"memchr(\"abcdef\", 'a', 6)",
		(STRPTR)"memchr must find a byte at the start of the region",
		p != (APTR)NULL && p == (APTR)"abcdef");

	p = memchr((APTR)"abcdef", (ULONG)'f', 6);
	ct_check_true((STRPTR)"mem", (STRPTR)"memchr_found_last",
		(STRPTR)"memchr(\"abcdef\", 'f', 6)",
		(STRPTR)"memchr must find a byte at the end of the region",
		p != (APTR)NULL && *((char *)p) == 'f');

	p = memchr((APTR)"abcdef", (ULONG)'z', 6);
	ct_check_ptr_null((STRPTR)"mem", (STRPTR)"memchr_missing",
		(STRPTR)"memchr(\"abcdef\", 'z', 6)",
		(STRPTR)"memchr must return NULL when the byte is not present",
		p);

	p = memchr((APTR)"abcdef", (ULONG)'c', 3);
	ct_check_true((STRPTR)"mem", (STRPTR)"memchr_limited_len",
		(STRPTR)"memchr(\"abcdef\", 'c', 3)",
		(STRPTR)"memchr must find a match within a limited search length",
		p != (APTR)NULL && *((char *)p) == 'c');

	p = memchr((APTR)"abcdef", (ULONG)'d', 3);
	ct_check_ptr_null((STRPTR)"mem", (STRPTR)"memchr_beyond_len",
		(STRPTR)"memchr(\"abcdef\", 'd', 3)",
		(STRPTR)"memchr must not search beyond the specified length",
		p);

	/* --- memmem --- */
	p = memmem((APTR)"abcdef", 6, (APTR)"cd", 2);
	ct_check_true((STRPTR)"mem", (STRPTR)"memmem_found",
		(STRPTR)"memmem(\"abcdef\", 6, \"cd\", 2)",
		(STRPTR)"memmem must find a matching byte sequence",
		p != (APTR)NULL && *((char *)p) == 'c');

	p = memmem((APTR)"abcdef", 6, (APTR)"xy", 2);
	ct_check_ptr_null((STRPTR)"mem", (STRPTR)"memmem_missing",
		(STRPTR)"memmem(\"abcdef\", 6, \"xy\", 2)",
		(STRPTR)"memmem must return NULL when the needle is absent",
		p);

	p = memmem((APTR)"abc", 3, (APTR)"", 0);
	ct_check_true((STRPTR)"mem", (STRPTR)"memmem_empty_needle",
		(STRPTR)"memmem(haystack, 3, \"\", 0)",
		(STRPTR)"memmem with empty needle must return the haystack pointer",
		p == (APTR)"abc");

	/* --- memccpy --- */
	memset(a, 0, sizeof(a));
	memset(b, 0, sizeof(b));
	strcpy(b, (STRPTR)"0123456789");
	p = memccpy(a, b, (ULONG)'5', 10);
	ct_check_true((STRPTR)"mem", (STRPTR)"memccpy_stop_char",
		(STRPTR)"memccpy(a, b, '5', 10)",
		(STRPTR)"memccpy must copy up to and including the stop character",
		a[0] == '0' && a[5] == '5' && p != (APTR)NULL);

	memset(a, 0, sizeof(a));
	memset(b, 0, sizeof(b));
	strcpy(b, (STRPTR)"ABC");
	p = memccpy(a, b, (ULONG)'Z', 3);
	ct_check_ptr_null((STRPTR)"mem", (STRPTR)"memccpy_no_stop",
		(STRPTR)"memccpy(a, \"ABC\", 'Z', 3)",
		(STRPTR)"memccpy must return NULL when the stop character is absent",
		p);

	memset(a, 0, sizeof(a));
	memset(b, 0, sizeof(b));
	strcpy(b, (STRPTR)"X");
	p = memccpy(a, b, (ULONG)'X', 1);
	ct_check_true((STRPTR)"mem", (STRPTR)"memccpy_one_byte",
		(STRPTR)"memccpy(a, \"X\", 'X', 1)",
		(STRPTR)"memccpy with stop at first byte must copy that byte and return pointer",
		a[0] == 'X' && p != (APTR)NULL);

	/* --- memswap --- */
	strcpy(a, (STRPTR)"AAAA");
	strcpy(b, (STRPTR)"BBBB");
	memswap(a, b, 4);
	ct_check_string((STRPTR)"mem",
		(STRPTR)"memswap_a",
		(STRPTR)"memswap(a, b, 4) buffer a",
		(STRPTR)"after memswap buffer a must hold the former contents of b",
		(STRPTR)"BBBB",
		a);
	ct_check_string((STRPTR)"mem",
		(STRPTR)"memswap_b",
		(STRPTR)"memswap(a, b, 4) buffer b",
		(STRPTR)"after memswap buffer b must hold the former contents of a",
		(STRPTR)"AAAA",
		b);

	strcpy(a, (STRPTR)"12");
	strcpy(b, (STRPTR)"34");
	memswap(a, b, 2);
	ct_check_string((STRPTR)"mem",
		(STRPTR)"memswap_two_bytes",
		(STRPTR)"memswap(a, b, 2) on two-byte buffers",
		(STRPTR)"memswap must exchange exactly n bytes between buffers",
		(STRPTR)"34",
		a);

	strcpy(a, (STRPTR)"OLD");
	strcpy(b, (STRPTR)"NEW");
	memswap(a, b, 3);
	memswap(a, b, 3);
	ct_check_string((STRPTR)"mem",
		(STRPTR)"memswap_double",
		(STRPTR)"double memswap restores original a",
		(STRPTR)"two memswap calls must restore the original buffer contents",
		(STRPTR)"OLD",
		a);

	/* --- sequential copies into separate buffers --- */
	memset(a, 0, sizeof(a));
	memset(b, 0, sizeof(b));
	memset(c, 0, sizeof(c));
	memcpy(a, (APTR)"seq1", 5);
	memcpy(b, (APTR)"seq2", 5);
	memcpy(c, (APTR)"seq3", 5);
	ct_check_true((STRPTR)"mem", (STRPTR)"sequential_copy_a",
		(STRPTR)"memcmp(a, \"seq1\", 5) after sequential memcpy",
		(STRPTR)"first sequential memcpy into buffer a must not affect other buffers",
		memcmp((APTR)a, (APTR)"seq1", 5) == 0);
	ct_check_true((STRPTR)"mem", (STRPTR)"sequential_copy_b",
		(STRPTR)"memcmp(b, \"seq2\", 5)",
		(STRPTR)"second sequential memcpy into buffer b must be independent",
		memcmp((APTR)b, (APTR)"seq2", 5) == 0);
	ct_check_true((STRPTR)"mem", (STRPTR)"sequential_copy_c",
		(STRPTR)"memcmp(c, \"seq3\", 5)",
		(STRPTR)"third sequential memcpy into buffer c must be independent",
		memcmp((APTR)c, (APTR)"seq3", 5) == 0);

	memset(a, 0, sizeof(a));
	memset(b, 0, sizeof(b));
	strcpy(src, (STRPTR)"source");
	memcpy(a, (APTR)src, 7);
	bcopy((APTR)src, b, 7);
	ct_check_long((STRPTR)"mem", (STRPTR)"memcpy_bcopy_agree",
		(STRPTR)"memcmp(a, b, 7) after memcpy and bcopy from same src",
		(STRPTR)"memcpy and bcopy from the same source must produce equal destinations",
		0, memcmp((APTR)a, (APTR)b, 7));

	memset(a, 0, sizeof(a));
	memset(b, 0, sizeof(b));
	memcpy(a, (APTR)src, 7);
	memmove(b, (APTR)src, 7);
	ct_check_long((STRPTR)"mem", (STRPTR)"memcpy_memmove_agree",
		(STRPTR)"memcmp(a, b, 7) after memcpy and memmove from same src",
		(STRPTR)"memcpy and memmove from the same source must produce equal destinations",
		0, memcmp((APTR)a, (APTR)b, 7));

	memset(a, 'Q', 10);
	memset(b, 'R', 10);
	memcpy(a, (APTR)"Z", 1);
	bcopy((APTR)"Z", b, 1);
	ct_check_true((STRPTR)"mem", (STRPTR)"sequential_single_byte_copies",
		(STRPTR)"single-byte memcpy and bcopy into separate prefilled buffers",
		(STRPTR)"single-byte copies into separate buffers must each set only the first byte",
		a[0] == 'Z' && b[0] == 'Z' && a[1] == 'Q' && b[1] == 'R');

	/* --- length edge cases (0 where safe) --- */
	r = memcmp((APTR)"a", (APTR)"b", 0);
	ct_check_long((STRPTR)"mem", (STRPTR)"edge_memcmp_zero",
		(STRPTR)"memcmp(\"a\", \"b\", 0)",
		(STRPTR)"memcmp with zero length must be safe and return 0",
		0, r);

	r = bcmp((APTR)"a", (APTR)"b", 0);
	ct_check_long((STRPTR)"mem", (STRPTR)"edge_bcmp_zero",
		(STRPTR)"bcmp(\"a\", \"b\", 0)",
		(STRPTR)"bcmp with zero length must be safe and return 0",
		0, r);

	memset(a, 'K', 4);
	ub = (UBYTE)a[0];
	memset(a, 'L', 0);
	ct_check_true((STRPTR)"mem", (STRPTR)"edge_memset_zero_len",
		(STRPTR)"memset(a, 'L', 0)",
		(STRPTR)"memset with zero length must not alter any bytes",
		(UBYTE)a[0] == ub);

	memset(a, 0, 0);
	ct_check_true((STRPTR)"mem", (STRPTR)"edge_memset_zero_fill_len",
		(STRPTR)"memset(a, 0, 0)",
		(STRPTR)"memset with zero length must not alter any bytes",
		(UBYTE)a[0] == ub);

	ub = (UBYTE)a[0];
	bzero(a, 0);
	ct_check_true((STRPTR)"mem", (STRPTR)"edge_bzero_zero_len",
		(STRPTR)"bzero(a, 0)",
		(STRPTR)"bzero with zero length must not alter any bytes",
		(UBYTE)a[0] == ub);

	memcpy(a, (APTR)"keep", 4);
	ub = (UBYTE)a[0];
	memcpy(b, a, 0);
	ct_check_true((STRPTR)"mem", (STRPTR)"edge_memcpy_zero_len",
		(STRPTR)"memcpy(b, a, 0)",
		(STRPTR)"memcpy with zero length must be safe and not copy bytes",
		(UBYTE)a[0] == ub);

	memset(b, 'R', 4);
	b[0] = 'P';
	bcopy((APTR)"src", b, 0);
	ct_check_true((STRPTR)"mem", (STRPTR)"edge_bcopy_zero_len",
		(STRPTR)"bcopy(\"src\", b, 0)",
		(STRPTR)"bcopy with zero length must be safe and not copy any bytes",
		b[0] == 'P');

	memset(b, 'R', 4);
	b[0] = 'P';
	memmove(b, (APTR)"src", 0);
	ct_check_true((STRPTR)"mem", (STRPTR)"edge_memmove_zero_len",
		(STRPTR)"memmove(b, \"src\", 0)",
		(STRPTR)"memmove with zero length must be safe and not copy any bytes",
		b[0] == 'P');

	strcpy(a, (STRPTR)"LEFT");
	strcpy(b, (STRPTR)"RIGHT");
	memswap(a, b, 0);
	ct_check_true((STRPTR)"mem", (STRPTR)"edge_memswap_zero_len",
		(STRPTR)"memswap(a, b, 0)",
		(STRPTR)"memswap with zero length must be safe and leave both buffers unchanged",
		strcmp(a, (STRPTR)"LEFT") == 0 && strcmp(b, (STRPTR)"RIGHT") == 0);
}
