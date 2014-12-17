

#define INITIAL_SIZE_MAP 100

typedef enum { false, true } bool;

struct entry
{
	word_t refutation;
	bool maximal = true;
};


typedef struct
{
	unsigned int size;
	struct entry *list;
}List;

List* createList();















