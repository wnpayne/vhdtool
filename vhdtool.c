#include<stdio.h>
#include<time.h>
#include<stdint.h>
#include<inttypes.h>
#include<unistd.h>
#include<endian.h>
#include<uuid/uuid.h>
#include<string.h>

/* Microsoft VHD file footer. 512 bytes, spec: http://download.microsoft.com/download/f/f/e/ffef50a5-07dd-4cf8-aaa3-442c0673a029/Virtual%20Hard%20Disk%20Format%20Spec_10_18_06.doc */

/* Unix time for January 1st 2000 UTC. The timestamp format is a big-endian 
   integer containing the number of seconds since January 1st 2000 UTC. */
#define WIN_REF_TIME 946713600
#define FOOTER_SIZE 512
#define WINDOWS 1466511979u
#define OSX 1298228000u

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
	unsigned char cOS[4];
	uint64_t originalsize;
	uint64_t currentsize;
	struct geo diskgeo;
	uint32_t disktype;
	uint32_t checksum;
	uuid_t uuid;
	unsigned char savedstate;
	unsigned char reserved[427]; 
};

// tested this recently and it was off by seemingly 1 hour from a windows box. Seems to work on my machine. maybe a daylight savings problem with UTC?
/* We get the VHD time by adding the unix time for January 1st 2000 to the seconds in
   dicated by the footer. The result is then converted to a human readable string. */
time_t read_timestamp(struct vhdfooter *in_footer)
{
	time_t sec;

	sec = (time_t) (WIN_REF_TIME + be32toh(in_footer->timestamp));
	return sec;
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
void footer_print(struct vhdfooter *in_footer)
{
	printf("Cookie:\t\t\t");
	printbytes(in_footer->cookie,8);
	printf("\nFeatures:\t\t");
	printbytes((char *) &in_footer->features,4);
	printf("\nFormat Version:\t\t");
	printbytes((char *) &in_footer->fileformat,4);
	printf("\nData Offset:\t\t");
	printbytes((char *) &in_footer->dataoffset,8);
	printf("\nTime Stamp:\t\t");
	printbytes((char *) &in_footer->timestamp,4);
	printf("\nCreator App:\t\t");
	printbytes(in_footer->cApp,4);
	printf("\nCreator Version:\t");
	printbytes((char *) &in_footer->cVer,4);
	printf("\nCreator OS:\t\t");
	printbytes((char *) &in_footer->cOS,4);
	printf("\nOriginal Size:\t\t");
	printbytes((char *) &in_footer->originalsize,8);
	printf("\nCurrent Size:\t\t");
	printbytes((char *) &in_footer->currentsize,8);
	printf("\nDisk Geometry:\t\t");
	printbytes((char *) &in_footer->diskgeo,4);
	printf("\nDisktype:\t\t");
	printbytes((char *) &in_footer->disktype,4);
	printf("\nChecksum:\t\t");
	printbytes((char *) &in_footer->checksum,4);
	printf("\nGUID:\t\t\t");
	printbytes((char *) &in_footer->uuid,16);
	printf("\nSaved State:\t\t");
	printbytes((char *) &in_footer->savedstate,1);
	printf("\n");
    
	//printf("structSize:\t%lu\n",sizeof(struct vhdfooter));
}

uint32_t footer_checksum(struct vhdfooter *in_footer)
{
	int i = 0;
	uint32_t checksum_neg = 0, oldchecksum = 0;
	unsigned char* char_ptr = 0;

	oldchecksum = in_footer->checksum;
	in_footer->checksum = 0;
	char_ptr = (unsigned char *) in_footer;

	for(i=0;i < FOOTER_SIZE;i++) {
		checksum_neg += (*char_ptr);
		char_ptr = char_ptr + 1;
	}
	in_footer->checksum = oldchecksum;
	return ~checksum_neg;	
}

void printbytes_guid(char *toprint, int len)
{
	int i=0;
	for (i=0;i<len;i++) {
		printf("%02X",(unsigned char) toprint[i]);
	}
}

void guid_print(uuid_t *inuuid)
{
	struct guid inguid =*((struct guid *) inuuid);
	inguid.data1  = htobe32(inguid.data1);
	inguid.data2 = htobe16(inguid.data2);
	inguid.data3  = htobe16(inguid.data3);

	printf("{");
	printbytes_guid((char *) &inguid.data1, 4);
	printf("-");
	printbytes_guid((char *) &inguid.data2, 2);
	printf("-");
	printbytes_guid((char *) &inguid.data3, 2);
	printf("-");
	printbytes_guid((char *) &inguid.data4, 2);
	printf("-");
	printbytes_guid((char *) &inguid.data4 + 2, 6);
	printf("}");
}

void listfields(struct vhdfooter *in_footer)
{
	uint32_t checksum = 0, ctimestamp = 0, disktype = 0;
	char fixed_cookie[9]="";
	char fixed_cApp[5]="";
	char fixed_cOS[5]="";
	time_t timestamp = (time_t) 0;
	
    	fixString(8,in_footer->cookie,fixed_cookie);
	fixString(4,in_footer->cApp,fixed_cApp);
	fixString(4,in_footer->cOS,fixed_cOS);
	checksum = footer_checksum(in_footer);
	timestamp = read_timestamp(in_footer);
	disktype = be32toh(in_footer->disktype);

	printf("Cookie:\t\t\t\"%s\"\n",fixed_cookie);
	printf("Features:\t\t%" PRIu32 "\n",be32toh(in_footer->features));
	printf("Format Version:\t\t%" PRIu32 "\n",be32toh(in_footer->fileformat));
	printf("Data Offset:\t\t%" PRIu64 "\n",be64toh(in_footer->dataoffset));
	printf("Created on:\t\t%s",ctime(&timestamp));
	printf("Creator App:\t\t\"%s\"\n",fixed_cApp);
	printf("Creator Version:\t%" PRIu32 "\n",be32toh(in_footer->cVer));
	printf("Creator OS:\t\t%s\n", fixed_cOS);
	printf("Original Size [b]:\t%" PRIu64 "\n", be64toh(in_footer->originalsize));
	printf("Current Size [b]:\t%" PRIu64 "\n", be64toh(in_footer->currentsize));
	printf("C/H/S:\t\t\t%" PRIu32 "/%" PRIu32 "/%" PRIu32 "\n", be16toh(in_footer->diskgeo.cylinders),
				in_footer->diskgeo.heads,in_footer->diskgeo.sectors);
	printf("Disktype:\t\t%");
	switch (disktype) {
	case 0:
		printf("None\n");
		break;
	case 1:
		printf("Deprecated\n");
		break;
	case 2:
		printf("Fixed hard disk\n");
		break;
	case 3:
		printf("Dynamic hard disk\n");
		break;
	case 4:
		printf("Differencing hard disk\n");
		break;
	case 5:
		printf("Deprecated\n");
		break;
	case 6:
		printf("Deprecated\n");
		break;
	default:
		printf("Unknown\n");
	}
	printf("Checksum:\t\t%" PRIu32 "\n", be32toh(in_footer->checksum));
	printf("Computed Checksum:\t%" PRIu32  "\n",checksum);
	printf("GUID:\t\t\t");
	guid_print(&in_footer->uuid);
	printf("\n");
	printf("Saved state:\t\t");
	if ( in_footer->savedstate )
		printf("Yes\n");
	else
		printf("No\n");
}

/* note: microsoft stores data1,data2,data3 in native endianess,
 * rather than Big Endian as with uuids */
int guid_gen(uuid_t *out_guid) {
	uuid_t new_uuid;
	struct guid *new_guid = (struct guid *) &new_uuid;
	struct guid *footer_guid = (struct guid *) out_guid;

	uuid_generate_time_safe(new_uuid);
	new_guid->data1 = be32toh(new_guid->data1);
	new_guid->data2 = be16toh(new_guid->data2);
	new_guid->data3 = be16toh(new_guid->data3);
	*footer_guid = *new_guid;
}

int creategeo(long filesize, struct geo *ingeo) {
	uint16_t ncylinders = 0;
	long totalsect = 0, cylinderxheads = 0;;
	char nheads = 0, nsectors = 0;

	totalsect = filesize/512;

	if (totalsect > 65535*16*255) {
		totalsect = 65535*16*255;
	}

	if (totalsect >= 65535*16*63) {
		nsectors = 255;
		nheads = 16;
		cylinderxheads = totalsect/nsectors;
	} else {
		nsectors = 17;
		cylinderxheads = totalsect/nsectors;
		nheads = (cylinderxheads + 1023)/1024;

		if (nheads < 4) {
			nheads = 4;
		}
		
		if (cylinderxheads >= (nheads * 1024) || nheads > 16) {
			nsectors = 31;
			nheads = 16;
			cylinderxheads = totalsect/nsectors;
		}	
		if (cylinderxheads >= (nheads * 1024)) {
			nsectors = 63;
			nheads = 16;
			cylinderxheads = totalsect/nsectors;
		}
	}
	ncylinders = cylinderxheads / nheads;

	ingeo->cylinders = htobe16(ncylinders);
	ingeo->heads = nheads;
	ingeo->sectors = nsectors;
	return 0;
}

int createfooter(struct vhdfooter *in_footer, char *inpath) {
	/*
	 * need to implement: 	customizable timestamp?
	 * 			functions for size -> geo
	 */

	FILE *infile;
	long filesize = 0;
	int i = 0;
	unsigned char ncookie[9] = "vhdtool!";
	unsigned char ncApp[5] = "wppt";
	unsigned char ncOS[5] = "Wi2k";

	infile = fopen(inpath,"rb");
	if (infile==NULL) {
		printf("bad file!");
	}
	else {
		fseek(infile,0,SEEK_END);
		filesize = ftell(infile);
		fclose(infile);

		memcpy(&in_footer->cookie,ncookie,8);
		in_footer->features = htobe32(2);
		in_footer->fileformat = htobe32(65536u);
		in_footer->dataoffset = htobe64(18446744073709551615u);
		in_footer->timestamp = htobe32(current_timestamp());
		memcpy(&in_footer->cApp,ncApp,4);
		in_footer->cVer = htobe32(393217u);
		memcpy(&in_footer->cOS,ncOS,4);
		in_footer->originalsize = htobe64(filesize);
		in_footer->currentsize = in_footer->originalsize;
		creategeo(filesize, &in_footer->diskgeo);
		in_footer->disktype = htobe32(2);
		guid_gen(&in_footer->uuid);
		in_footer->savedstate = 0;
		for(i=0;i<427;i++) {
			in_footer->reserved[i] = 0;
		}
		
	}

	return 0;
}

int outputfooter(struct vhdfooter *in_footer, char *outpath)
{
	FILE *outfile;
	
	outfile = fopen(outpath,"ab");
	fwrite(in_footer,FOOTER_SIZE,1,outfile);
	fclose(outfile);
	
	return 0;
}

int main(int argc, char *argv[])
{
	FILE *infile;
	struct vhdfooter footer;
	int arg, hflag = 0, lflag = 0, cflag = 0, oflag = 0;
	char *ovalue = NULL;
	uint32_t checksum;


	while ((arg = getopt(argc, argv, "hlo:c")) != -1) {
		switch (arg) {
		case 'h':
			hflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'o':
			oflag = 1;
			ovalue = optarg;
		}
	}

	if (argc = 1)
		lflag = 1;

	if (oflag && !cflag)
		lflag = 1;

	if (hflag || lflag && !cflag) {
		infile=fopen(argv[optind],"rb");

		if (!infile) {
			printf("bad file! improve this message and handling...\n");
			return 1;
		}
		fseek(infile,0,SEEK_END);
		if (ftell(infile) < 512) {
			printf("file too short\n");
			return 1;
		}
		fseek(infile,-512,SEEK_END);
		fread(&footer,FOOTER_SIZE,1,infile);
		fclose(infile);

		if (hflag && lflag) {
			listfields(&footer);
			printf("\n");
			footer_print(&footer);
		}
		else if (lflag) {
			listfields(&footer);
		}
		else {
			footer_print(&footer);
		}
	}

	if (oflag || cflag) {
		if (oflag && cflag) {
			createfooter(&footer,argv[optind]);
		}
		else if (cflag) {
			ovalue = argv[optind];
			createfooter(&footer,argv[optind]);
		}

		if (!lflag)
			footer.checksum = htobe32(footer_checksum(&footer));
		outputfooter(&footer, ovalue); 
	}

	return 0;
}
