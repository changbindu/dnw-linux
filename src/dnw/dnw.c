#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

const char* dev = "/dev/secbulk0";
#define BLOCK_SIZE	(1*1024*1024)

struct download_buffer {
	uint32_t	load_addr;  /* load address */
	uint32_t	size; /* data size */
	uint8_t		data[0];
	/* uint16_t checksum; */
};

static int _download_buffer(struct download_buffer *buf)
{
	int fd_dev = open(dev, O_WRONLY);
	if( -1 == fd_dev) {
		printf("Can not open %s: %s\n", dev, strerror(errno));
		return -1;
	}

	printf("Writing data...\n");
	size_t remain_size = buf->size;
	size_t block_size = BLOCK_SIZE;
	size_t writed = 0;
	while(remain_size>0) {
		size_t to_write = remain_size > block_size ? block_size : remain_size;
		if( to_write != write(fd_dev, (unsigned char*)buf + writed, to_write)) {
			perror("write failed");
			close(fd_dev);
			return -1;
		}
		remain_size -= to_write;
		writed += to_write;
		printf("\r%02zu%%\t0x%08zX bytes (%zu K)",
			(size_t)((uint64_t)writed*100/(buf->size)),
			writed,
			writed/1024);
		fflush(stdout);
	}
	printf("\n");
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

static struct download_buffer* alloc_buffer(size_t data_size)
{
	struct download_buffer	*buffer = NULL;
	size_t total_size = data_size + sizeof(struct download_buffer) + 2;

	buffer = (typeof(buffer))malloc(total_size);
	if(NULL == buffer)
		return NULL;
	buffer->size = total_size;
	return buffer;
}

static void free_buffer(struct download_buffer *buf)
{
	free(buf);
}

static struct download_buffer *load_file(const char *path, unsigned long load_addr)
{
	struct stat		file_stat;
	struct download_buffer	*buffer = NULL;
	unsigned long		total_size;
	int			fd;

	fd = open(path, O_RDONLY);
	if(-1 == fd) {
		printf("Can not open file %s: %s\n", path, strerror(errno));
		return NULL;
	}

	if( -1 == fstat(fd, &file_stat) ) {
		perror("Get file size filed!\n");
		goto error;
	}	

	buffer = alloc_buffer(file_stat.st_size);
	if(NULL == buffer) {
		perror("malloc failed!\n");
		goto error;
	}
	if( file_stat.st_size !=  read(fd, buffer->data, file_stat.st_size)) {
		perror("Read file failed!\n");
		goto error;
	}

	buffer->load_addr = load_addr;
	cal_and_set_checksum(buffer);

	return buffer;

error:
	if(fd != -1)
		close(fd);
	if( NULL != buffer )
		free(buffer);
	return NULL;
}

static int download_file(const char *path, unsigned long load_addr)
{
	struct download_buffer *buffer;
	struct timeval __start, __end;
	long __time_val = 0;
	float speed = 0.0;

	buffer = load_file(path, load_addr);
	gettimeofday(&__start,NULL);
	if (buffer != NULL) {
		if (_download_buffer(buffer) == 0) {
			gettimeofday(&__end,NULL);
			__time_val = (long)(__end.tv_usec - __start.tv_usec)/1000 + \
				(long)(__end.tv_sec - __start.tv_sec) * 1000;
			speed = (float)buffer->size/__time_val/(1024*1024) * 1000;
			printf("speed: %fM/S\n",speed);
			free_buffer(buffer);
		} else {
			free_buffer(buffer);
			return -1;
		}
	} else
		return -1;
}

int main(int argc, char* argv[])
{
	unsigned load_addr = 0x57e00000;
	char* path = NULL;
	int	c;

	while ((c = getopt (argc, argv, "a:h")) != EOF)
	switch (c) {
	case 'a':
		load_addr = strtol(optarg, NULL, 16);
		continue;
	case '?':
	case 'h':
	default:
usage:
		printf("Usage: dwn [-a load_addr] <filename>\n");
		printf("Default load address: 0x57e00000\n");
		return 1;
	}
	if (optind < argc)
		path = argv[optind];
	else
		goto usage;

	printf("load address: 0x%08X\n", load_addr);
	if (download_file(path, load_addr) != 0) {
		return -1;
	}

	return 0;
}

