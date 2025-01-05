#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <io.h>
#include <memoryapi.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#include "mapped_file.h"
#include "greatest/greatest.h"


// Test allocating a mapped file
TEST test_mapped_file_new() {
    size_t size = 1024;
    size_t alignment = 16;
    mapped_file *mapped_file = mapped_file_new_alignment(size, alignment);

    ASSERT(mapped_file != NULL);
    ASSERT(mapped_file->region.data != NULL);
    ASSERT(((uintptr_t)mapped_file->region.data % alignment) == 0);

    mapped_file_destroy(mapped_file);
    PASS();
}

TEST test_mapped_file_map_fd() {
    const char *filename = "testfile.txt";
    const char *content = "Test for mapped_file with file descriptor!";
    size_t size = strlen(content) + 1;

    // Create a test file
    FILE *file = fopen(filename, "w");
    ASSERT(file != NULL);
    fputs(content, file);
    fclose(file);

    #ifdef _WIN32
    int fd = _open(filename, _O_RDONLY);
    #else
    int fd = open(filename, O_RDONLY);
    #endif
    ASSERT(fd != -1);

    mapped_file *mapped_file = mapped_file_map_from_file_descriptor(fd, 0, size);

    ASSERT(mapped_file != NULL);
    ASSERT(mapped_file->region.data != NULL);
    ASSERT(memcmp(mapped_file->region.data, content, size) == 0);

    close(fd);
    mapped_file_destroy(mapped_file);
    remove(filename);
    PASS();
}

// Test reading file into memory
TEST test_mapped_file_map() {
    const char *filename = "testfile.txt";
    const char *content = "Fallback test for mapped_file!";

    // Create a test file
    FILE *file = fopen(filename, "w");
    ASSERT(file != NULL);
    fputs(content, file);
    fclose(file);

    size_t size = strlen(content);
    FILE *stream = fopen(filename, "r");
    ASSERT(stream != NULL);

    bool memory_map = true;
    mapped_file *mapped_file = mapped_file_map(stream, memory_map, filename, size);

    ASSERT(mapped_file != NULL);
    ASSERT(mapped_file->region.data != NULL);
    ASSERT(memcmp(mapped_file->region.data, content, size) == 0);

    fclose(stream);
    mapped_file_destroy(mapped_file);
    remove(filename);
    PASS();
}


// Test fallback to reading file into memory
TEST test_mapped_file_map_fallback() {
    const char *filename = "testfile_fallback.txt";
    const char *content = "Fallback test for mapped_file!";

    // Create a test file
    FILE *file = fopen(filename, "w");
    ASSERT(file != NULL);
    fputs(content, file);
    fclose(file);

    size_t size = strlen(content);
    FILE *stream = fopen(filename, "r");
    ASSERT(stream != NULL);

    bool memory_map = false;
    mapped_file *mapped_file = mapped_file_map(stream, memory_map, filename, size);

    ASSERT(mapped_file != NULL);
    ASSERT(mapped_file->region.data != NULL);
    ASSERT(memcmp(mapped_file->region.data, content, size) == 0);

    fclose(stream);
    mapped_file_destroy(mapped_file);
    remove(filename);
    PASS();
}

// Run all tests
SUITE(mapped_file_suite) {
    RUN_TEST(test_mapped_file_new);
    RUN_TEST(test_mapped_file_map);
}

GREATEST_MAIN_DEFS();
int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(mapped_file_suite);
    GREATEST_MAIN_END();
}
