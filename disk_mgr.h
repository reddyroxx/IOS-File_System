#define _LARGEFILE64_SOURCE

#define GIG (1024 * 1024 * 1024)
#include <sys/types.h>
int disk_init(char *df_path);

int disk_get_block(int fileno, off_t block_offset, char *io_buf);

int disk_set_block(int fileno, off_t block_offset, char *io_buf);

void disk_destroy(void);