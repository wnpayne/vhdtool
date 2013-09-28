#include<stdio.h>
#include<string.h>

typedef struct vhdfooter
{
    unsigned char cookie[8];
    unsigned int features;
    unsigned int fileformat;
    unsigned long int dataoffset;
    unsigned int timestamp;
    unsigned char cApp[4];
    unsigned int cVer[4];
    unsigned int cOS[4];
    unsigned long int originalsize;
    unsigned long int currentsize;
    unsigned int diskgeo;
    unsigned int disktype;
    unsigned int checksum;
    unsigned char uuid[16];
    unsigned char savedstate;
    unsigned char reserved[427];

}VHDFOOTER;

int main()
{
    FILE *myfile;
    VHDFOOTER footer;

    myfile=fopen("/mnt/test1foot","rb");
    	
    fread(&footer,sizeof (VHDFOOTER),1,myfile);
    fclose(myfile);
    printf("%lu\n",sizeof(footer.dataoffset));

    return 0;
}
