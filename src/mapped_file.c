#include "mapped_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include "aligned/aligned.h"

#ifdef _WIN32
#define DWORD_MAX 0xFFFFFFFF
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

static memory_region create_memory_region(void *data, void *mmap, size_t size, size_t offset) {
    memory_region region = {
        .data = data,
        .mmap = mmap,
        .size = size,
        .offset = offset
    };
    #ifdef _WIN32
    region.file_mapping = NULL;
    #endif
    return region;
}

mapped_file *mapped_file_new_from_region(memory_region region) {
    mapped_file *file = malloc(sizeof(mapped_file));
    if (file == NULL) return NULL;
    file->region = region;
    return file;
}


// Allocate memory with alignment
mapped_file *mapped_file_new_alignment(size_t size, size_t alignment) {
    void *data = aligned_malloc(size, alignment);
    if (data == NULL) return NULL;

    memory_region region = create_memory_region(data, NULL, size, 0);
    return mapped_file_new_from_region(region);
}

mapped_file *mapped_file_new(size_t size) {
    return mapped_file_new_alignment(size, MAPPED_FILE_MIN_ALIGNMENT);
}



mapped_file *mapped_file_map_from_file_descriptor(int fd, size_t pos, size_t size) {
    const long page_size = get_page_size();

    const size_t offset = pos % page_size;
    const size_t offset_pos = pos - offset;
    const size_t upsize = size + offset;

#ifdef _WIN32
    if (fd == -1) {
        fprintf(stderr, "Invalid file descriptor fd=%d\n", fd);
        return NULL;
    }
    int file = _get_osfhandle((intptr_t)fd);
    if (file == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Invalid file descriptor fd=%d\n", fd);
        return NULL;
    }
    const DWORD max_size_hi =
        sizeof(size_t) > sizeof(DWORD) ? upsize >> (CHAR_BIT * sizeof(DWORD)) : 0;
    const DWORD max_size_lo = upsize & DWORD_MAX;
    HANDLE file_mapping = CreateFileMappingA((HANDLE)file, NULL, PAGE_READONLY,
                                            max_size_hi, max_size_lo, NULL);
    if (file_mapping == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Can't create file mapping for fd=%d size=%zu: %lu\n",
                fd, upsize, GetLastError());
        return NULL;
    }

    const DWORD offset_pos_hi =
        sizeof(size_t) > sizeof(DWORD) ? offset_pos >> (CHAR_BIT * sizeof(DWORD))
                                        : 0;
    const DWORD offset_pos_lo = offset_pos & DWORD_MAX;
    void *map = MapViewOfFile(file_mapping, FILE_MAP_READ,
                                offset_pos_hi, offset_pos_lo, upsize);
    if (map == NULL) {
        fprintf(stderr, "Can't map file for fd=%d size=%zu offset=%zu: %lu\n",
                fd, upsize, offset_pos, GetLastError());
        CloseHandle(file_mapping);
        return NULL;
    }
#else
    void *map = mmap(NULL, upsize, PROT_READ, MAP_SHARED, fd, offset_pos);
    if (map == MAP_FAILED) {
        fprintf(stderr, "Can't map file for fd=%d size=%zu offset=%zu\n",
                fd, upsize, offset_pos);
        return NULL;
    }
#endif

    memory_region region;
    region.mmap = map;
    region.size = upsize;
    region.data = (void *)((char *)map + offset);
    region.offset = offset;
#ifdef _WIN32
    region.file_mapping = file_mapping;
#endif
    return mapped_file_new_from_region(region);
}

// Map a file from an input stream
mapped_file *mapped_file_map(FILE *stream, bool memory_map, const char *source, size_t size) {
    long stream_pos = ftell(stream);
    if (memory_map && stream_pos >= 0 && stream_pos % MAPPED_FILE_MIN_ALIGNMENT == 0) {
        size_t pos = (size_t)stream_pos;
#ifdef _WIN32
        int fd = _open(source, _O_RDONLY);
#else
        int fd = open(source, O_RDONLY);
#endif
        if (fd != -1) {
            mapped_file *mmf = mapped_file_map_from_file_descriptor(fd, pos, size);
            if (close(fd) == 0 && mmf != NULL) {
                fseek(stream, pos + size, SEEK_SET);
                if (ftell(stream) == (long)(pos + size)) {
                    return mmf;
                }
            } else {
                fprintf(stderr, "Mapping of file failed: %s\n", strerror(errno));
            }
        }
    }

    fprintf(stderr, "File mapping at offset %ld of file %s could not be honored, reading instead\n", stream_pos, source);

    mapped_file *mf = mapped_file_new(size);
    if (!mf) return NULL;

    char *buffer = (char *)mf->region.data;
    while (size > 0) {
        size_t next_size = size > MAPPED_FILE_MAX_READ_CHUNK ? MAPPED_FILE_MAX_READ_CHUNK : size;
        long current_pos = ftell(stream);
        if (fread(buffer, 1, next_size, stream) != next_size) {
            fprintf(stderr, "Failed to read %zu bytes at offset %ld from \"%s\"\n", next_size, current_pos, source);
            mapped_file_destroy(mf);
            return NULL;
        }
        size -= next_size;
        buffer += next_size;
    }
    return mf;
}


void mapped_file_destroy(mapped_file *file) {
    if (file == NULL) return;
#ifdef _WIN32
    if (file->region.data) UnmapViewOfFile(file->region.data);
    if (file->region.mmap) CloseHandle(file->region.mmap);
#else
    if (file->region.data) munmap(file->region.data, file->region.size);
#endif
    free(file);
}

