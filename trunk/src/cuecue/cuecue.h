
#define BLOCK_SIZE 0x10000;

struct MusicReader
{
	
	
};

int Open(char * filename);
int Close(char * filename);
int Read(void* buffer, int size);