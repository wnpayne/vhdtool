#include<stdio.h>
#include<time.h>
#include<stdint.h>
#include<endian.h>

/* Microsoft VHD file footer. 512 bytes, spec: http://download.microsoft.com/download/f/f/e/ffef50a5-07dd-4cf8-aaa3-442c0673a029/Virtual%20Hard%20Disk%20Format%20Spec_10_18_06.doc */

/* Unix time for January 1st 2000 UTC. The timestamp format is a big-endian 
   integer containing the number of seconds since January 1st 2000 UTC. */
#define WIN_REF_TIME 946713600
#define FOOTER_SIZE 512

struct vhdfooter {
	unsigned char cookie[8];
	uint32_t features;
	uint32_t fileformat;
	uint64_t dataoffset;
	uint32_t timestamp;
	unsigned char cApp[4];
	uint32_t cVer;
	uint32_t cOS;
	uint64_t originalsize;
	uint64_t currentsize;
	uint32_t diskgeo;
	uint32_t disktype;
	uint32_t checksum;
	unsigned char uuid[16];
	unsigned char savedstate;
	unsigned char reserved[427]; 
};

// tested this recently and it was off by seemingly 1 hour from a windows box. Seems to work on my machine. maybe a daylight savings problem with UTC?
/* We get the VHD time by adding the unix time for January 1st 2000 to the seconds in
   dicated by the footer. The result is then converted to a human readable string. */
void read_timestamp(struct vhdfooter in_footer)
{

	time_t sec;

	sec = (time_t) (WIN_REF_TIME + be32toh(in_footer.timestamp));
	printf(ctime(&sec));

}

uint32_t current_timestamp(void)
{
	time_t sec;
	uint32_t timestamp = 0;
	
	sec = time(NULL);
	timestamp = (uint32_t) (sec) - WIN_REF_TIME;

	return timestamp;

}

/* provide (1) original string without a null terminator.
   provide its length in originalLen;
   provide (1) empty string WITH a null terminator.
   its length should be originalLen+1 */
void fixString(int originalLen, char *originalString, char *fixedString)
{
	int i;

	for(i=0;i<originalLen;i++)
	{
		fixedString[i]=originalString[i];
	}

}

/* provide a pointer to a "string" (actually just a memory address), and the number of bytes to print. */
void printbytes(char *toprint, int len)
{

	int i=0;
	for (i=0;i<len;i++) {
		printf("%02X ",(unsigned char) toprint[i]);
	}

}

/* Pretty print the (mostly) raw bytes of the footer. */
void footer_print(struct vhdfooter in_footer)
{
	char fixed_cookie[9]="";
	char fixed_cApp[5]="";
	
    	fixString(8,in_footer.cookie,fixed_cookie);
	fixString(4,in_footer.cApp,fixed_cApp);
	printf("cookie:\t\"%s\"\ncapp:\t\"%s\"",fixed_cookie,fixed_cApp);
	printf("\nFeatures:\t");
	printbytes((char *) &in_footer.features,4);
	printf("\nFile Format:\t");
	printbytes((char *) &in_footer.fileformat,4);
	printf("\nCreatorr Ver:\t");
	printbytes((char *) &in_footer.cVer,4);
	printf("\nCreator HostOS:\t");
	printbytes((char *) &in_footer.cOS,4);
	printf("\nData Offset:\t");
	printbytes((char *) &in_footer.dataoffset,8);
	printf("\nTime Stamp:\t");
	printbytes((char *) &in_footer.timestamp,4);
	printf("\nOriginal Size:\t");
	printbytes((char *) &in_footer.originalsize,8);
	printf("\nCurrent Size:\t");
	printbytes((char *) &in_footer.currentsize,8);
	printf("\nDisk Geo:\t");
	printbytes((char *) &in_footer.diskgeo,4);
	printf("\nDisk Type:\t");
	printbytes((char *) &in_footer.disktype,4);
	printf("\nChecksum:\t");
	printbytes((char *) &in_footer.checksum,4);
	printf("\nSaved State:\t");
	printbytes((char *) &in_footer.savedstate,1);
	printf("\nuuid:\t\t");
	printbytes((char *) &in_footer.uuid,16);
	printf("\n\n");
    
	printf("structSize:\t%lu\n",FOOTER_SIZE);
}

uint32_t footer_checksum(struct vhdfooter in_footer)
{
	uint32_t checksum = 0, checksum_comp = 0;
	int i = 0, j = 0, k = 0;
	unsigned char* char_ptr = 0;

	in_footer.checksum = 0;
	char_ptr = (unsigned char *) &in_footer;

	for(i=0;i < FOOTER_SIZE;i++) {
		checksum += (*char_ptr);
		char_ptr = char_ptr + 1;
//		printf("%u %u %u\n",i,*char_ptr,checksum);
	}
	
	//return *(unsigned int *) &checksum_comp;	
	return ~checksum;	
//	return checksum;
}

int main(int argc, char *argv[])
{
	FILE *myfile;
	int i=0;
	uint32_t checksum = 0, ctimestamp = 0;
	struct vhdfooter footer;

	myfile=fopen(argv[1],"rb");

	fread(&footer,FOOTER_SIZE,1,myfile);
	fclose(myfile);

	footer_print(footer);

	printf("given checksum: %u\n", be32toh(footer.checksum));

	checksum = footer_checksum(footer);
	printf("computed checksum: %u\n",checksum);
	printf("\n");
	read_timestamp(footer);
	
	ctimestamp = current_timestamp();
	printf("%u\t%u\n", ctimestamp, be32toh(ctimestamp));

	/* the timestamp needs to be stored as big-endian, so switch before writing.
	   again stop using checksum variable */

	/* test writing a new header with changed timestamp */

	footer.timestamp = htobe32(ctimestamp);
	
	myfile = fopen("./output-test","wb");
	fwrite(&footer,FOOTER_SIZE,1,myfile);
	fclose(myfile);

	printf("size in bytes: %llu\n", be64toh(footer.originalsize));

    return 0;
}
