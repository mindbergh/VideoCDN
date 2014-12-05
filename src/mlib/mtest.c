#include <mlib/mlist.h>
#include <mlib/mslist.h>
#include <mlib/mqueue.h>
#include <mlib/mdebug.h>
#include <stdlib.h>
#include <stdio.h>


struct test_s {
	int a;
};

void print_test(void* data, void* func_data);

int main(int argc, char** argv) {
	struct test_s *test = (struct test_s*)malloc(sizeof(struct test_s));
	struct test_s *test2 = (struct test_s*)malloc(sizeof(struct test_s));
	struct test_s *test3 = (struct test_s*)malloc(sizeof(struct test_s));
	test->a = 1;
	test2->a = 2;
	test3->a = 3;
	MList* list = mlist_new();
	list->data = (void*)test3;
	list = mlist_append(list, (void*)test);
	list = mlist_append(list, (void*)test2);
	mlist_foreach(list, print_test, NULL);
}


void print_test(void* data, void* func_data) {
	struct test_s* test = (struct test_s*)data;
	printf("This is a list and a = %d\n", test->a);
	return;
}

