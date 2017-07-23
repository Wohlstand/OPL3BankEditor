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
    unsigned short ins_count = 0;
    fseek(f1, 2, SEEK_SET);
    fread(&ins_count, 2, 1, f1);
    fseek(f1, 0, SEEK_SET);
    dataPos = (ins_count * 9) + 6;
    while( valid1 && valid2 )
    {
        valid1 = (fread(&byte1, 1, 1, f1)!=0);
        valid2 = (fread(&byte2, 1, 1, f2)!=0);

        if(!valid1 && !valid2)
            break;

        if(count==0)
        {
            printf("Header\n");
        }

        if(count==6)
        {
            printf("Header END\n");
        }

        if( (count >= 6) && (count < dataPos))
        {
            int relPos = (count - 6) % 9;
            if(relPos == 0)
                printf("Title\n");
        }

        if(count >= dataPos)
        {
            int relPos = (count - dataPos) % 56;
            if(relPos == 0 )
                printf("INSTRUMENT!!! %d\nModulator\n", insCount++);
            if(relPos == 26 )
                printf("Carrier\n");

            if((relPos == 4)||(relPos == (26+4)))
                printf("---feedback\n");

            if(relPos == 26 * 2 )
                printf("WaveForms\n");
        }

        printf("%03X: {0x%02x == 0x%02x}%s\n", count, byte1, byte2, (byte1==byte2)?"":" <- NOT SAME!!!");
        count++;
    }
}
