#include <iostream>
#include <fstream>
#include <windows.h>

#define CHAR_H      29       
#define CHAR_W      16       // szerokosc pojedynczego znaku (stala)
#define CHAR_NO     96       // ilosc znakow
#define WIDTH       (CHAR_W*CHAR_NO)
#define BYTE_SIZE   ((WIDTH*CHAR_H)>>3) // rozmiar w bajtach (1 bajt = 8 bitow)

void fprintf_bin(FILE* f, char* beginning, char* ending, unsigned short val)
{
    fprintf(f,beginning);
    fprintf(f,"%d /* ",val);
    for(int i=15;i>=0;i--)
        fprintf(f,"%d",(val>>i)&1);
    fprintf(f," */");
    fprintf(f,ending);
}

int main()
{
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    RGBQUAD bmiColors[2];
    char data[BYTE_SIZE];

    FILE* f = fopen("font.bmp","rb");
    if(!f)
        return 1;
    if(fread(&bfh,1,sizeof(BITMAPFILEHEADER),f)!=sizeof(BITMAPFILEHEADER)) {
        fclose(f);
        return 2;
    }
    if(fread(&bih,1,sizeof(BITMAPINFOHEADER),f)!=sizeof(BITMAPINFOHEADER)) {
        fclose(f);
        return 3;
    }
    if(bih.biWidth!=WIDTH||bih.biHeight!=CHAR_H) {
        fclose(f);
        return 4;
    }
    if(bih.biBitCount!=1) {
        fclose(f);
        return 5;
    }
    if(fread(&bmiColors,1,2*sizeof(RGBQUAD),f)!=2*sizeof(RGBQUAD)) {
        fclose(f);
        return 6;
    }
    if(fread(data,1,BYTE_SIZE,f)!=BYTE_SIZE) {
        fclose(f);
        return 7;
    }
    
    FILE* out = fopen("../src/font.h","w");
    if(!out)
        return 8;
    fprintf(out,"/** (C) Maciej Muszkowski 2010 */\n");
    fprintf(out,"#define CHAR_H 29\n"); 
    fprintf(out,"const unsigned short font_data[%d][%d] = {\n", CHAR_NO, CHAR_H);
    
    for(int char_no=0;char_no<CHAR_NO;char_no++)
    {
        fprintf(out,"{\n");
        for(int y=CHAR_H-1;y>=0;y--)
        {
            unsigned short hi = (data[(y*(WIDTH>>3)) + char_no*(CHAR_W>>3)])<<8;
            unsigned char low =  data[(y*(WIDTH>>3)) + char_no*(CHAR_W>>3) + 1];
            unsigned short val = hi|low;
            
            if(y!=0)
                fprintf_bin(out,"\t",",\n",val);
            else
                fprintf_bin(out,"\t","\n",val);
        }
        if(char_no!=CHAR_NO-1)
            fprintf(out,"},\n");
        else
            fprintf(out,"}\n");
    }
    
    fprintf(out,"};\n");    
    fclose(out);
    
    fclose(f);
    return 0;
}
