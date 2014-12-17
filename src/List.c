

#include "List.h"

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

List* createList()
{
	List* refutations = (List*) malloc(sizeof(List));
	refutations->entry = (entry*) malloc(sizeof(entry) * INITIAL_SIZE_MAP);
	refutations->size = INITIAL_SIZE_MAP;
	return refutations;
}
