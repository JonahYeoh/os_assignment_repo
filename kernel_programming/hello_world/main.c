#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>

MODULE_LICENSE("GPL");								/* License */
MODULE_AUTHOR("Jonah");							/* Who wrote this module? */
MODULE_DESCRIPTION("This kernel module print out current time");		/* What does this module do */

int start_module(void){ // init_module
	/* 
		<linux/time.h> 
		https://stackoverflow.com/questions/13552536/get-current-time-in-seconds-in-kernel-module
	*/
	struct timespec t;
	getnstimeofday(&t);
	printk( KERN_ALERT "Kernel Module Loaded Successfully\t2020-10-14T%ld:%ld\n", (t.tv_sec%86400/3600)+8, t.tv_sec%3600/60 );
	return 0;
}

void clean_up(void){
	printk( KERN_INFO "Kernel Module Removed Successfully\t107316154\tJonah Yeoh\n" );
}

module_init(start_module);
module_exit(clean_up);
