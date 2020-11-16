#ifndef ERROR_DESCRIPTION_H
#define ERROR_DESCRIPTION_H

/* 
* ================================================== ERROR output functions ==================================================
*/
char *get_error_description(const unsigned int index);

char *get_error_description(const unsigned int index)
{
	switch(index){
		case 1:
			return "Memory Allocation Error";
		case 2:
			return "Memory Reallocation Error";
		case 3:
			return "Empty Command";
		case 4:
			return "Invalid Input";
		case 5:
			return "Write To Shared Memory Failure";
		case 6:
			return "Incomplete Parameters";
		case 7:
			return "Address range exceed 32 bits limit";
		default:
			return "Unknown Error";
	}
}

#endif