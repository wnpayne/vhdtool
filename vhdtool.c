#include<stdio.h>

//Microsoft VHD file footer. 512 bytes, spec: http://download.microsoft.com/download/f/f/e/ffef50a5-07dd-4cf8-aaa3-442c0673a029/Virtual%20Hard%20Disk%20Format%20Spec_10_18_06.doc

typedef struct vhdfooter
{
    unsigned char cookie[8];
    unsigned int features;
    unsigned int fileformat;
    unsigned long int dataoffset;
    unsigned int timestamp;
    unsigned char cApp[4];
    unsigned int cVer;
    unsigned int cOS;
    unsigned long int originalsize;
    unsigned long int currentsize;
    unsigned int diskgeo;
    unsigned int disktype;
    unsigned int checksum;
    unsigned char uuid[16];
    unsigned char savedstate;
    unsigned char reserved[427]; 

}VHDFOOTER;

//provide (1) original string without a null terminator.
//provide its length in originalLen;
//provide (1) empty string WITH a null terminator.
//its length should be originalLen+1
void fixString(int originalLen, char *originalString, char *fixedString) {
    int i;

    for(i=0;i<originalLen;i++)
    {
       fixedString[i]=originalString[i];
    }

}

void printbits(char *toprint,int len) {

    int i=0;
    for (i=0;i<len;i++) {
        printf("%02X ",(unsigned char) toprint[i]);
    }

}

void footer_print(VHDFOOTER in_footer) {
	char fixed_cookie[9]="";
	char fixed_cApp[5]="";
	
    	fixString(8,in_footer.cookie,fixed_cookie);
	fixString(4,in_footer.cApp,fixed_cApp);
	printf("cookie:\t\"%s\"\ncapp:\t\"%s\"",fixed_cookie,fixed_cApp);
	printf("\nFeatures\t");
	printbits((char *) &in_footer.features,4);
	printf("\nFile Format\t");
	printbits((char *) &in_footer.fileformat,4);
	printf("\nCreatorr Ver:\t");
	printbits((char *) &in_footer.cVer,4);
	printf("\nCreator HostOS:\t");
	printbits((char *) &in_footer.cOS,4);
	printf("\nData Offset:\t");
	printbits((char *) &in_footer.dataoffset,8);
	printf("\nTime Stamp\t");
	printbits((char *) &in_footer.timestamp,4);
	printf("\nOriginal Size\t");
	printbits((char *) &in_footer.originalsize,8);
	printf("\nCurrent Size\t");
	printbits((char *) &in_footer.currentsize,8);
	printf("\nDisk Geo:\t");
	printbits((char *) &in_footer.diskgeo,4);
	printf("\nDisk Type\t");
	printbits((char *) &in_footer.disktype,4);
	printf("\nChecksum\t");
	printbits((char *) &in_footer.checksum,4);
	printf("\nSaved State:\t");
	printbits((char *) &in_footer.savedstate,1);
	printf("\nuuid:\t");
	printbits((char *) &in_footer.uuid,16);
	printf("\n\n");
    
	printf("structSize:\t%lu\n",sizeof(VHDFOOTER));
}

int main(int argc, char *argv[]) {
    FILE *myfile;
    int i=0;
    VHDFOOTER footer;

    myfile=fopen(argv[1],"rb");
    	
    fread(&footer,sizeof (VHDFOOTER),1,myfile);
    fclose(myfile);

    footer_print(footer);

    return 0;
}
