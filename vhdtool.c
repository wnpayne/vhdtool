#include<stdio.h>
#include<time.h>
#include<stdint.h>
#include<unistd.h>
#include<endian.h>
#include<uuid/uuid.h>

/* Microsoft VHD file footer. 512 bytes, spec: http://download.microsoft.com/download/f/f/e/ffef50a5-07dd-4cf8-aaa3-442c0673a029/Virtual%20Hard%20Disk%20Format%20Spec_10_18_06.doc */

/* Unix time for January 1st 2000 UTC. The timestamp format is a big-endian 
   integer containing the number of seconds since January 1st 2000 UTC. */
#define WIN_REF_TIME 946713600
#define FOOTER_SIZE 512

struct geo {
	uint16_t cylinders;
	uint8_t heads;
	uint8_t sectors;
};

struct guid {
	uint32_t data1;
	uint16_t data2;
	uint16_t data3;
	uint64_t data4;
};

/* define uuid as byte array to avoid padding issues */

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
	struct geo diskgeo;
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
	uint32_t checksum_neg = 0;
	int i = 0, j = 0, k = 0;
	unsigned char* char_ptr = 0;

	in_footer.checksum = 0;
	char_ptr = (unsigned char *) &in_footer;

	for(i=0;i < FOOTER_SIZE;i++) {
		checksum_neg += (*char_ptr);
		char_ptr = char_ptr + 1;
	}
	
	return ~checksum_neg;	
}

void printbytes_guid(char *toprint, int len)
{
	int i=0;
	for (i=0;i<len;i++) {
		printf("%02X",(unsigned char) toprint[i]);
	}
}

void guid_print(struct vhdfooter in_footer)
{
	struct guid *inguid = (struct guid *) &in_footer.uuid;
	uint32_t fd1 = 0;
	uint16_t fd2 = 0, fd3 = 0;
	uint64_t fd4 = 0;
	
	fd1 = htobe32(inguid->data1);
	fd2 = htobe16(inguid->data2);
	fd3 = htobe16(inguid->data3);
	fd4 = inguid->data4;

	printf("GUID:\t{");
	printbytes_guid((char *) &fd1, 4);
	printf("-");
	printbytes_guid((char *) &fd2, 2);
	printf("-");
	printbytes_guid((char *) &fd3, 2);
	printf("-");
	printbytes_guid((char *) &fd4, 2);
	printf("-");
	printbytes_guid((char *) &fd4 + 2, 6);
	printf("}");
	printf("\n");
}

void print_geo(struct vhdfooter in_footer)
{
	printf("disk geometry: %u/%u/%u\n", be16toh(in_footer.diskgeo.cylinders),
				in_footer.diskgeo.heads,in_footer.diskgeo.sectors);
}

int createvhd(struct vhdfooter in_footer) {
	/*
	 * things to calculate:
	 * 	timestamp,  original size, current size, disk geo, checksum, uuid
	 * things to invent:
	 * 	cookie, capp, creator ver
	 * given:
	 * 	features, format, hostos, offset, disk type, saved state
	 *
	 *
	 * need to implement: 	customizable timestamp?
	 * 			size input (eventual detection)
	 * 			functions for size -> geo
	 * 			generating uuid
	 * 			hooking in extant checksum fn.
	 */

	uuid_t uuid1;
	struct guid *guid1 = (struct guid *) &uuid1;
	struct guid *guid2 = (struct guid *) &in_footer.uuid;

	uuid_generate_time_safe(uuid1);
	guid1->data1 = be32toh(guid1->data1);
	guid1->data2 = be16toh(guid1->data2);
	guid1->data3 = be16toh(guid1->data3);
	*guid2 = *guid1;
	guid_print(in_footer);


	return 0;
}

void listfields(struct vhdfooter in_footer)
{
	uint32_t checksum = 0, ctimestamp = 0;

	footer_print(in_footer);

	printf("given checksum: %u\n", be32toh(in_footer.checksum));

	checksum = footer_checksum(in_footer);
	printf("computed checksum: %u\n",checksum);
	printf("\n");
	read_timestamp(in_footer);
	
	ctimestamp = current_timestamp();
	printf("%u\t%u\n", ctimestamp, be32toh(ctimestamp));

	/* the timestamp needs to be stored as big-endian, so switch before writing.
	 * test writing a new header with changed timestamp */

	/* output testing:
	in_footer.timestamp = htobe32(ctimestamp);
	
	myfile = fopen("./output-test","wb");
	fwrite(&in_footer,FOOTER_SIZE,1,myfile);
	fclose(myfile); */

	printf("size in bytes: %llu\n", be64toh(in_footer.originalsize));
	guid_print(in_footer);
	printf("\n");

	print_geo(in_footer);
}

int outputfooter(struct vhdfooter in_footer, char *outpath)
{
	FILE *outfile;

	outfile = fopen(outpath,"wb");
	fwrite(&in_footer,FOOTER_SIZE,1,outfile);
	fclose(outfile);
	
	return 0;
}

int main(int argc, char *argv[])
{
	FILE *myfile;
	struct vhdfooter footer;
	int arg, hflag = 0, lflag = 0, cflag = 0;
	char *cvalue = NULL;

	myfile=fopen(argv[argc - 1],"rb");

	fread(&footer,FOOTER_SIZE,1,myfile);
	fclose(myfile);

	while ((arg = getopt(argc, argv, "hlo:c")) != -1) {
		switch (arg) {
		case 'h':
			printf("h flag!\n");
			break;
		case 'l':
			listfields(footer);
			break;
		case 'c':
			createvhd(footer);
			break;
		case 'o':
			printf("o flag!\n");
			cvalue = optarg;
			printf("%s\n",cvalue);
			outputfooter(footer, cvalue);
			break;
		default:
			printf("no options!");
		}
	}

    return 0;
}
