#include <stdio.h>

int main()
{
    FILE* f1 = fopen("bnk1.bnk", "rb");
    FILE* f2 = fopen("bnk2.bnk", "rb");

    unsigned char byte1=0, byte2=0;
    int count=0;
    int insCount=0;
    int dataPos = 4;
    int valid1 = 1;
    int valid2 = 1;
    int drumMode = 0;
    while( valid1 && valid2 )
    {
        valid1 = (fread(&byte1, 1, 1, f1)!=0);
        valid2 = (fread(&byte2, 1, 1, f2)!=0);

        if(!valid1 && !valid2)
            break;
        /*
        if(count==0)
        {
            printf("Header\n");
        }
        if(count==4)
        {
            printf("Header END\n");
        }
        if(count>=dataPos)
        {
            if((count-dataPos)%16==0)
            {
                if(drumMode)
                    printf("DRUM!!! %d\n", (insCount++)-128);
                else
                    printf("INSTRUMENT!!! %d\n", insCount++);
                if(insCount==128)
                    drumMode = 1;
            }
        }*/
        printf("%03X: {0x%02x == 0x%02x}%s\n", count, byte1, byte2, (byte1==byte2)?"":" <- NOT SAME!!!");
        count++;
    }
}
