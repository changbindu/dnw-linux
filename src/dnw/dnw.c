#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

const char* dev = "/dev/secbulk0";

struct download_buffer {
	uint32_t	load_addr;  /* load address */
	uint32_t	size; /* data size */
	uint8_t		data[0];
	/* uint16_t checksum; */
};

static int _download_buffer(struct download_buffer *buf)
{
	int fd_dev = open(dev, O_WRONLY);
	if( -1 == fd_dev)
		printf("Can not open %s\n", dev);

	printf("Writing data...\n");
	size_t remain_size = buf->size;
	size_t block_size = remain_size / 100;
	size_t writed = 0;
	while(remain_size>0) {
		size_t to_write = remain_size > block_size ? block_size : remain_size;
		if( to_write != write(fd_dev, (unsigned char*)buf + writed, to_write)) {
			printf("failed!\n");
			close(fd_dev);
			return -1;
		}
		remain_size -= to_write;
		writed += to_write;
		printf("\r%u%%\t %u bytes", writed*100/(buf->size), writed);
		fflush(stdout);
	}
	close(fd_dev);
	return 0;
}

static inline void cal_and_set_checksum(struct download_buffer *buf)
{
	uint16_t sum = 0;
	int i;

	for(i = 0; i < buf->size; i++) {
		sum += buf->data[i];
	}
	*((uint16_t*)(&((uint8_t*)buf)[buf->size - 2])) = sum;
}

static int download_file(const char *path, unsigned long load_addr)
{
	struct stat		file_stat;
	struct download_buffer	*buffer = NULL;
	unsigned long		total_size;
	int			fd;

	fd = open(path, O_RDONLY);
	if(-1 == fd) {
		printf("Can not open file - %s\n", path);
		return -1;
	}

	if( -1 == fstat(fd, &file_stat) ) {
		printf("Get file size filed!\n");
		goto error;
	}	
	
	total_size = file_stat.st_size + sizeof(struct download_buffer) + 2;
	buffer = (typeof(buffer))malloc(total_size);
	if(NULL == buffer) {
		printf("malloc failed!\n");
		goto error;
	}
	if( file_stat.st_size !=  read(fd, buffer->data, file_stat.st_size)) {
		printf("Read file failed!\n");
		goto error;
	}

	buffer->load_addr = load_addr;
	buffer->size = total_size;
#if 0
	cal_and_set_checksum(buffer);
#endif
	return _download_buffer(buffer);

error:
	if(fd != -1)
		close(fd);
	if( NULL != buffer )
		free(buffer);
	return -1;
}

int main(int argc, char* argv[])
{
	if( 2 != argc )	{
		printf("Usage: dwn <filename>\n");
		return 1;
	}

	if (download_file(argv[1], 0x32000000) != 0) {
		return -1;
	}

	printf("OK\n");
	return 0;
}

