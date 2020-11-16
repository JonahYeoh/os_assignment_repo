#ifndef INTEL_HEX_H
#define INTEL_HEX_H

/* 
* ================================================== INTEL-HEX-FORMAT's specific functions ==================================================
*/

/*
* ====================================================== API ======================================================
* int convert(const char *src, const char *dst);
* purpose 		: convert binary file <src> into intel-hex-format and write to file <dst>
* assumption	: <src> file are assummed as binary file, old <dst> file will be replaced
* arguements	: src = name of source file, dst = name of destination file
* return 		: always return 3 for correct release of shared memory, output error message to stderr if error
*
* unsigned short int checksum(const unsigned char dc, const unsigned short int offset, const unsigned char rtype, const char data[], const int dlen );
* purpose		: compute intel-hex specific checksum
* arguements	: bc = byte_count, offset = address_offset_low, rtype = record_type, data = data_array, dlen = data_length
* return 		: 16-bits unsigned integer
*/

unsigned short int checksum(const unsigned char bc, const unsigned short int offset, const unsigned char rtype, const char data[], const int dlen )
{
	unsigned short int upper, lower;
	upper = offset >> 8;
	lower = offset << 8;
	lower >>= 8;
	unsigned int sum = (unsigned short int)bc + upper + lower + (unsigned short int)rtype;
	int i;
	for ( i = 0; i < dlen; i++ )	sum += (unsigned short int)data[i];
	sum = ~sum;
	sum += 1;
	return sum;
}

int convert(const char *src, const char *dst)
{
	FILE *read_ptr = NULL;
	if ( ( read_ptr = fopen(src, "rb")) == NULL )
	{
		fprintf(stderr, "File I/O Exception: Read Error\n");
		return 3;
	}
	FILE *write_ptr = NULL;
	if ( ( write_ptr = fopen(dst, "w")) == NULL )
	{
		fprintf(stderr, "File I/O Exception: Write Error\n");
		return 3;
	}
	unsigned char byte;
	fseek(read_ptr, 0, SEEK_SET);
	int read_count, prev = 0;
	unsigned char record[33]; memset(record, 0, 33);
	int i = 0, k = 0, seq = 0;
	unsigned short int base = 0X0000;
	unsigned short int address;
	address = base;
	unsigned short int csum;
	fprintf(write_ptr, ":020000040000FA\n");
	while ( fread(&byte, 1, 1, read_ptr) == 1 )
	{
		if ( i < 32 )	record[i++] = byte;
		else
		{
			csum = checksum(0x20, address, 0X00, record, 32);
			csum = (csum<<8);
			csum = (csum>>8); 
			// printf("%d\t:20%04X00", seq++, address);
			fprintf(write_ptr, ":20%04X00", address);
			for ( k = 0; k < 32; k++ )
			{
				// printf("%02X", record[k]);
				fprintf(write_ptr, "%02X", record[k]);
			}
			// printf("%02X\n", csum);
			fprintf(write_ptr, "%02X\n",csum);
			memset(record, 0, 33);
			i = 0;
			record[i++] = byte;
			
			if ( address == 0XFFE0 )
			{
				address = 0;
				base++;
				if(base==0XFFFF){
					fprintf(stderr, "Exception : Address range exceed 32 bits limit\n");
					return 3;
				}
				// printf("%d\t:02000004%04XF9\n", seq++, base);
				fprintf(write_ptr, ":02000004%04XF9\n", base);
			}
			else	address += 32;
		}
		
	}
	csum = checksum(i, address, 0X00, record, i);
	csum = (csum<<8);
	csum = (csum>>8); 
	// printf("%d\t:%02X%04X00", seq++, i, address);
	fprintf(write_ptr, ":%d%04X00", i, address);
	for ( k = 0; k < i; k++ )
	{
		// printf("%02X", record[k]);
		fprintf(write_ptr, "%02X", record[k]);
	}
	// printf("%02X\n", csum);
	fprintf(write_ptr, "%02X\n",csum);
	// printf("%d\t:00000001FF\n", seq);
	fprintf(write_ptr, ":00000001FF\n");
	fprintf(stderr, "Task Completed : use command less <HEX_FILENAME>.hex to view converted file.\n");
	fclose(read_ptr);
	fclose(write_ptr);
	return 3;
}

#endif