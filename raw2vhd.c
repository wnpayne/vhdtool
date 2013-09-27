#include<stdio.h>

struct rec
{
    int x,y;
};

int main()
{
    int counter;
    FILE *myfile;
    struct rec my_record;

    myfile=fopen("./test.bin","rb");
    if(!myfile)
    {
        printf("Unable to open file");
        return 1;
    }
    my_record.x = 1;
    my_record.y = 7;
    fwrite(&my_record,sizeof(struct rec), 1, myfile);
    fclose(myfile);
    return 0;
}
