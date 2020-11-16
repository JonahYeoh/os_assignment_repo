#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/random.h>
/*
	<linux/list.h>
	url : https://gist.github.com/gmcclure/4080371
	Description : Ref to implementation of function included
*/
static LIST_HEAD(score_list);	// Make variable score_list the head

struct student{		// Self Defined Structure
	char name[20];		// Name
	int nat_sci;		// Natural Science
	int math;		// Mathematic
	int nat_lang;		// National Language
	int avg;		// Average Score
	char remainer[2];	// Remainer
	struct list_head list; // Pointer to the next node
};

void set_decimal_value(const int rem, char buffer[2]){
	// printk(KERN_INFO "REM -> %d\n", rem );
	/*
		SSE disabled, FPU not allowed in kernel module
		manually compute decimal digits through the remainer
	*/
	// Lazy approach
	if ( rem == 1 )
		{buffer[0] = '3'; buffer[1] = '3';}
	else if ( rem == 2 )
		{buffer[0] = '6'; buffer[1] = '7';}
	else
		{buffer[0] = '0'; buffer[1] = '0';}
	/*
	// When more variety of output are expected, rounding up not possible
	int j, variable = rem;
	for ( j = 0; j < 2; j++ ){
		variable *= 10;
		buffer[j] = '0' + (variable / 3);
		variable %= 3;
	}
	*/
}

void get_random_buffer(char buffer[3], const int N){
	/*
		<linux/random.h>
		url : http://www.polarhome.com/service/man/?qf=get_random_bytes&tf=2&of=OSF1&sf=
		API : void get_random_bytes( ref buffer, int bytes );
	*/
	get_random_bytes(buffer, N);
}

int exam_start(void){
	/* Declaration Segment */
	struct student *pupil;
	int i;int c; int temp;
	char S_BUFFER[11] = { 0 };
	char R_BUFFER[3] = { 0 };
	/* Assignment Segment */
	for( i = 1; i < 6; i++ ){
		// learn more about GFP_KERNEL -> https://www.kernel.org/doc/htmldocs/kernel-api/API-kmalloc.html
		/*
			<linux/slab.h>
			url : https://www.kernel.org/doc/htmldocs/kernel-api/API-kmalloc.html
			API : void * kmalloc( size_t size, gfp_t flags );
			Description : allocate memory with broader access
		*/
		pupil = kmalloc(sizeof(*pupil), GFP_KERNEL);	// allocate memory space
		for ( c = 0; c < 11; c++ )
			pupil->name[c] = 0;
		get_random_buffer(S_BUFFER, 11);	// fetch 
		for ( c = 0; c < 10; c++ )		// asign name
			pupil->name[c] = 'A' + (unsigned int)( S_BUFFER[c] ) % 26;
		pupil->nat_sci = (unsigned int)( S_BUFFER[0] ) % 100;
		pupil->math = (unsigned int)( S_BUFFER[1] ) % 100;
		pupil->nat_lang = (unsigned int)( S_BUFFER[2] ) % 100;
		temp = (pupil->nat_sci + pupil->math + pupil->nat_lang);
		pupil->avg = temp/3;
		
		/*
			<linux/list.h>
			url : https://www.kernel.org/doc/html/v4.14/core-api/kernel-api.html#c.list_add
			API : void list_add_tail(struct list_head * new, struct list_head * head)
			      static inline void __list_add(struct list_head *new,
			      	struct list_head *prev,
			      	struct list_head *next)
			      	Implementation:
			      		next->prev = new;
					new->next = next;
					new->prev = prev;
					prev->next = new;
			Description : Insert a new entry after the specified head. Think doublely linked-list
		*/
		list_add_tail(&pupil->list, &score_list);
	}
	printk(KERN_INFO "Module Loaded Successfully\n");
	/* Output Segment */
	list_for_each_entry(pupil, &score_list, list){
		temp = (pupil->nat_sci + pupil->math + pupil->nat_lang) % 3;
		set_decimal_value(temp, R_BUFFER);
		printk( KERN_INFO "Name %s, Natural Science ->%3d, Mathematic ->%3d, National Language ->%3d, AVG ->%3d.%s\n", pupil->name, pupil->nat_sci, pupil->math, pupil->nat_lang, pupil->avg, R_BUFFER );
	}
	// End Of Program
	return 0;
}

void exam_end(void){
	/* Declaration Segment */
	struct student *pupil, *next;
	/* Release Segment */
	/*
		<linux/list.h>
		url : https://www.kernel.org/doc/htmldocs/kernel-api/API-list-for-each-entry-safe.html
		API : void list_for_each_entry_safe( *pos, *n, *head, *member )
		Description : iterate over list of given type safe against removal of list entry 
	*/
	/*
		list_for_each_entry VS list_for_each_entry_safe
		#define list_for_each_entry(pos, head, member)
     		for (pos = list_entry((head)->next, typeof(*pos), member);
     			pos->member != (head); 
     				pos = list_entry(pos->member.next, typeof(*pos), member))
     		#define list_for_each_entry_safe(pos, n, head, member)               /
     		for (pos = list_entry((head)->next, typeof(*pos), member), n = list_entry(pos->member.next, typeof(*pos), member); 
     			pos->member != (head);
     				pos = n, n = list_entry(n->member.next, typeof(*n), member))	
		Observation : We can see that list_for_each_entry_safe() use additional pointer variable to capture the pointer to the next node.
		With this thoughtful consideration, we can safely iterate through the list with/without removing the current node
	*/
	list_for_each_entry_safe(pupil, next, &score_list, list)
	{
		printk(KERN_INFO "Releasing Node => %s\n", pupil->name);
		list_del(&pupil->list);	// release inner pointer
		kfree(pupil);			// release node pointer
	}
	printk(KERN_INFO "Module Removed Successfully\n");
	// End Of Program
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("KERNEL MODULE WITH LIST");
MODULE_AUTHOR("JONAH");

module_init( exam_start );
module_exit( exam_end );
