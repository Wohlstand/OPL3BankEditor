#include <stdio.h>

int main()
{
    FILE* f1 = fopen("tim.snd", "rb");
    FILE* f2 = fopen("tim.tim", "rb");

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

        printf("%03X: {0x%02x == 0x%02x}%s\n", count, byte1, byte2, (byte1==byte2)?"":" <- NOT SAME!!!");
        count++;
    }
}

