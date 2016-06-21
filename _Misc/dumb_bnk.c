
#include <stdio.h>

#define BNK_HEAD_OFFSET 8

#define SIZEOF_NAME     12
#define SIZEOF_INST     30


int read16LE(FILE *f, unsigned short *target)
{
    unsigned char bytes[2];
    int got = fread(bytes, 1, 2, f);
    *target  = bytes[0];
    *target |= (((unsigned short)bytes[1])<<8) & 0xFF00;
    return got;    
}

int read32LE(FILE *f, unsigned int *target)
{
    unsigned char bytes[4];
    int got = fread(bytes, 1, 4, f);
    *target  = bytes[0];
    *target |= (((unsigned short)bytes[1])<<8) & 0xFF00;
    *target |= (((unsigned short)bytes[2])<<16) & 0xFF0000;
    *target |= (((unsigned short)bytes[3])<<24) & 0xFF000000;
    return got;
}

int main(int arvc, char**argv)
{
    if(arvc<2)
    {
        printf("U r sux!\n");
        return 1;
    }
    //FILE* bnk = fopen("insmaker_standard.bnk", "rb");
    FILE* bnk = fopen(argv[1], "rb");
    if(!bnk)
    {
        printf("CANT OPEN!\n");
        return -1;
    }
    unsigned short numIns,   numInsUsed;
    unsigned int   nameOffset, dataOffset;
    
    fseek(bnk, BNK_HEAD_OFFSET, SEEK_SET);
    if( read16LE(bnk, &numInsUsed) != 2 )
    {
        printf("CANT READ 1!\n");
        return -1;
    }
    if( read16LE(bnk, &numIns) != 2 )
    {
        printf("CANT READ 2!\n");
        return -1;
    }
    if( read32LE(bnk, &nameOffset) != 4 )
    {
        printf("CANT READ 3!\n");
        return -1;
    }
    if( read32LE(bnk, &dataOffset) != 4 )
     {
        printf("CANT READ 4!\n");
        return -1;
    }
    
    unsigned short i=0;
    for(i=0; i<numIns; i++)
    {
        fseek(bnk, nameOffset+SIZEOF_NAME*i, SEEK_SET);
        unsigned short insIndex;
        unsigned short insAddress;
        unsigned char  flags;
        unsigned char  ins[SIZEOF_INST];
        unsigned char  name[9];
        if( read16LE(bnk, &insIndex) != 2 )
        {
            printf("I CAN'T READ INSTRUMENT NAME SLOT :(\n");
            return -1;
        }
        if( fread(&flags, 1, 1, bnk) != 1 )
            return -1;
        if( fread(name, 1, 9, bnk) != 9 )
            return -1;
        int j=0;
        for(j=0; j<8; j++)
        {
            if(name[j] < 0x21 || name[j] >= 127 )
                name[j] = '?';
        }
        name[8] = '\0';
        
        printf("[%-8s] FLAGS: 0x%02X, IDX: %05d => ", name, flags, insIndex);
        insAddress = dataOffset + (insIndex*SIZEOF_INST);
        fseek(bnk, insAddress, SEEK_SET);
        if(fread(ins, 1, SIZEOF_INST, bnk) != SIZEOF_INST)
        {
            printf("I CAN'T READ INSTRUMENT :(\n");
            return -1;
        }

        printf("P=0x%02X VN=0x%03d "
               "{OP1 L=0x%02X M=0x%02X F=0x%03X A=0x%02X S=0x%02X E=0x%02X D=0x%02X r=0x%02X l=0x%02X A=0x%02X V=0x%02X R=0x%02X C=0x%02X} "
               "{OP2 L=0x%02X M=0x%02X F=%03d A=0x%02X S=0x%02X E=0x%02X D=0x%02X r=0x%02X l=0x%02X A=0x%02X V=0x%02X R=0x%02X C=0x%02X} "
               "W1=0x%02X W2=0x%02X\n",

        ins[0], ins[1],

        ins[2],  ins[3],  ins[4],  ins[5],  ins[6],  ins[7],  ins[8],  ins[9],  ins[10], ins[11], ins[12], ins[13], ins[14],
        ins[15], ins[16], ins[17], ins[18], ins[19], ins[20], ins[21], ins[22], ins[23], ins[24], ins[25], ins[26], ins[27],

        ins[28], ins[29]);
    }
    fclose(bnk);
    return 0;
}
