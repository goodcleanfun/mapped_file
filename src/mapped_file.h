#ifndef MAPPED_FILE_H
#define MAPPED_FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define MAPPED_FILE_UNDEF_WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifdef MAPPED_FILE_UNDEF_WIN32_LEAN_AND_MEAN
#undef MAPPED_FILE_UNDEF_WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
#endif


// A memory region is a simple abstraction for allocated memory or data from
// memory-mapped files. If mmap is null, then data represents an owned region
// of size bytes. Otherwise, mmap and size refer to the mapping and data is a
// casted pointer to a region contained within [mmap, mmap + size). If size is
// 0, then mmap and data refer to a block of memory managed externally by some
// other allocator. The offset is used when allocating memory to providing
// padding for alignment.

typedef struct {
    void *data;       // Pointer to the memory region
    void *mmap;       // Pointer to the mmap region (or file mapping handle on Windows)
    size_t size;      // Size of the memory region
    size_t offset;    // Offset within the memory region
#ifdef _WIN32
    HANDLE file_mapping; // Windows-specific file mapping handle
#endif
} memory_region;

typedef struct {
    memory_region region; // Memory region managed by this mapped file
} mapped_file;

#define MAPPED_FILE_MIN_ALIGNMENT 16
#define MAPPED_FILE_MAX_READ_CHUNK (256 * 1024 * 1024)

mapped_file *mapped_file_new(size_t size);
mapped_file *mapped_file_new_alignment(size_t size, size_t alignment);
mapped_file *mapped_file_new_from_region(memory_region region);
mapped_file *mapped_file_map(FILE *stream, bool memory_map, const char *source, size_t size);
mapped_file *mapped_file_map_from_file_descriptor(int fd, size_t pos, size_t size);
void mapped_file_destroy(mapped_file *file);

#endif // MAPPED_FILE_H