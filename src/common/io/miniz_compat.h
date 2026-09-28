#ifndef MINIZ_COMPAT_H
#define MINIZ_COMPAT_H

/* AURORA_GBA_ARCHIVE_SINK_V1: this header is included before any system
   header in miniz_compat.c, so pull stddef.h for size_t here. */
#include <stddef.h>

/* Minimal helpers that replace the gz / zip APIs the SNESticle code
   used to call into zlib + minizip-unzip. Backed by miniz instead.
   Both helpers go through newlib stdio (fopen / fread), which after
   init_ps2_filesystem_driver() resolves through iomanX, so PS2 paths
   like cdfs:/, host:/, mass:/ and mc0:/ all work transparently. */

#ifdef __cplusplus
extern "C" {
#endif

/* Reads a `.gz` file at `path`, decompresses the deflate stream into
   `out_buf`, returning the number of decompressed bytes (>0) or -1
   on any failure (open / parse / decompress). At most `out_max`
   bytes are written. */
int MinizReadGZToBuffer(const char *path,
                        void *out_buf,
                        int out_max);

/* Opens the zip at `path`, walks the central directory and decompresses
   the first non-directory entry whose name is accepted by `name_filter`
   (or the first entry if `name_filter` is NULL) into `out_buf`.
   Returns the uncompressed byte count (>0) or -1 if no entry matched
   or any miniz / IO step failed. The matched file's name (within the
   archive) is written into `out_filename` if non-NULL, truncated to
   `filename_max` bytes incl. NUL. */
int MinizReadZipFirstMatch(const char *path,
                           void *out_buf,
                           int out_max,
                           char *out_filename,
                           int filename_max,
                           int (*name_filter)(const char *name));

/* AURORA_DYNAMIC_ROM_BUFFER_V1_20260823 */
int MinizGetGZUncompressedSize(const char *path);
int MinizReadGZPrefix(const char *path, void *out_buf, int out_max);

typedef int (*MinizZipEntryFilter)(
    const char *name, unsigned int uncompressed_size);

int MinizProbeZipFirstMatchInfo(const char *path,
                                unsigned int *out_file_index,
                                char *out_filename,
                                int filename_max,
                                MinizZipEntryFilter entry_filter);

int MinizReadZipEntryToBuffer(const char *path,
                              unsigned int file_index,
                              void *out_buf,
                              int out_max,
                              char *out_filename,
                              int filename_max);

int MinizReadZipEntryPrefix(const char *path,
                            unsigned int file_index,
                            void *out_buf,
                            int out_max);

/* AURORA_GBA_ZIP_STREAM_V1_20260910
 * Streaming extraction straight to a file, for ROMs larger than the EE
 * heap can hold alongside the emulator cores. Neither helper ever holds
 * the whole uncompressed image in memory: ZIP goes through miniz's
 * extract-to-callback with a small fwrite sink, GZ runs a chunked
 * tinfl loop (32 KiB in / 32 KiB out). Returns the uncompressed byte
 * count written to out_path, or -1 on any failure (in which case
 * out_path has been removed). */
int MinizExtractZipEntryToFile(const char *path,
                               unsigned int file_index,
                               const char *out_path,
                               char *out_filename,
                               int filename_max);

int MinizExtractGZToFile(const char *path,
                         const char *out_path);

/* AURORA_GBA_ARCHIVE_SINK_V1_20260911
 * Sink-based variants for the GBA memory cart (ZIP/GZ entries extracted
 * straight into gpSP's ROM cache window). The sink receives sequential
 * uncompressed chunks with their absolute output offset and must return
 * the number of bytes accepted (anything else aborts the extraction).
 * file_ofs is 64-bit on purpose: it must match miniz's mz_uint64
 * callback ABI on this target.
 * All allocations these functions make come from BSS statics, because
 * they run AFTER gpSP's retro_init has claimed up to 16 MiB of heap for
 * the ROM cache, where even a 64 KiB heap malloc is not guaranteed. */
typedef size_t (*MinizSinkFn)(void *opaque,
                              unsigned long long file_ofs,
                              const void *buf,
                              size_t n);

int MinizExtractZipEntryToSink(const char *path,
                               unsigned int file_index,
                               MinizSinkFn sink,
                               void *opaque,
                               char *out_filename,
                               int filename_max);

int MinizExtractGZToSink(const char *path,
                         MinizSinkFn sink,
                         void *opaque);

/* Early-stopping prefix probes for the single-slot ROM cache identity
 * check: decompress only the first out_max bytes of the entry, then stop
 * reading the archive. Returns out_max when that many bytes could be
 * produced, otherwise -1 (or a smaller count when the stream ends
 * first - callers treat anything != out_max as a miss). */
int MinizReadZipEntryPrefixEarly(const char *path,
                                 unsigned int file_index,
                                 void *out_buf,
                                 int out_max);

int MinizReadGZPrefixStreaming(const char *path,
                               void *out_buf,
                               int out_max);

#ifdef __cplusplus
}
#endif

#endif /* MINIZ_COMPAT_H */
