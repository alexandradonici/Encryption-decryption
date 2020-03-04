#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
}pixel; //structura ce stocheaza intensitatea fiecarui canal de culoare R,G,B

typedef union
{
    unsigned int x;
    unsigned char b[4];
}Bytes; //uniune ce permite reprezentarea unui numar pe octeti

void dim( int *latime,int *inaltime, char * cale_imagine)//transmite ca parametri de iesire latimea si inaltimea imaginii, exprimate in numar de pixeli
{

 FILE *fin=fopen(cale_imagine,"rb");

 if(fin==NULL)
 {
     printf("Imaginea initiala nu a fost gasita.");
     return;
 }

   fseek(fin,18, SEEK_SET);
   fread(latime, sizeof(int), 1, fin);//latimea este memorata pe patru octeti fara semn incepand cu octetul al 18-lea din header
   fread(inaltime, sizeof(int), 1, fin);//inaltimea este memorata pe patru octeti fara semn incepand cu octetul al 22-lea din header

   fclose(fin);

}

 void header(char * cale_imagine, char **h)//salveaza header-ul unei imagini
{

    int i;

    FILE *fin=fopen(cale_imagine, "rb");
    if(fin==NULL)
 {
     printf("Imaginea nu a fost gasita");
     return;
 }

  *h=(char *)malloc(55);
  if(h==NULL)
  {
      printf("Nu s-a putut aloca memorie pentru vectorul h.");
      return;
  }
    for(i=0;i<54;i++) //header-ul ocupa primii 54 de octeti din fisier
        fread(&(*h)[i],1,1,fin);

    fclose(fin);


}

int padding( int latime)//determina numarul octetilor de padding
{
    if(latime%4!=0)
        return 4-(3*latime)%4;

    return 0;
}

void liniarizare(char * cale_imagine, pixel **p, int latime, int inaltime)//încarcă în memoria internă o imagine de tip BMP în formă liniarizată
{
    *p=(pixel *)malloc(inaltime*latime*sizeof(pixel));
      if(*p==NULL)
  {
      printf("Nu s-a putut aloca memorie pentru vectorul p.");
      return;
  }

    FILE *fin=fopen(cale_imagine, "rb");
     if(fin==NULL)
 {
     printf("Imaginea nu a fost gasita");
     return;
 }

    pixel x;
    int i,j,pad;

    pad=padding(latime);

    fseek(fin,54,SEEK_SET);//se sare peste header
    for(i=inaltime-1;i>=0;i--)
    {
        for(j=0;j<latime;j++)
        {
            fread(&x.blue,sizeof(char),1,fin);
            fread(&x.green,sizeof(char),1,fin);
            fread(&x.red,sizeof(char),1,fin);
            //octetii se citesc de la dreapta la stanga deoarece fisierul binar respecta standardul little-endian
            (*p)[i*latime+j]=x;//se tine cont ca imaginea este memorata invers in fisierul binar, octetii fiind salvati pe pozitia corespunzatoare in vector
        }
        fseek(fin,pad,SEEK_CUR);// se sare peste octetii de padding de la sfarsitul fiecarei linii
    }

    fclose(fin);
}

void deliniarizare(char *img_finala, pixel *p,int latime,int inaltime,char *h)//salveaza in memoria externa o imagine BMP stocata in forma liniarizata in memoria interna
{
    FILE *fout=fopen(img_finala, "wb");
     if(fout==NULL)
 {
     printf("Eroare la deschiderea fisierului binar");
     return;
 }

    int i,j,q,pad;
    pixel x;
    unsigned char y=0;

    pad=padding(latime);

    for(i=0;i<54;i++)
        fwrite(&h[i],sizeof(char),1,fout);

    for(i=inaltime-1;i>=0;i--)
       {
           for(j=0;j<latime;j++)
    {
        x=p[i*latime+j];

        fwrite(&x.blue,sizeof(char),1,fout);
        fwrite(&x.green,sizeof(char),1,fout);
        fwrite(&x.red,sizeof(char),1,fout);
    }
    for(q=0;q<pad;q++)
             fwrite(&y,1,1,fout);

       }

    fclose(fout);
}

void XORSHIFT32(unsigned int **r, unsigned int r0,int latime, int inaltime)//genereaza o secventa de numere intregi aleatoare fara semn pe 32 de biti
{
    int k,i;
    unsigned int x;

    k=2*inaltime*latime-1;//k reprezinta numarul de elemente ce trebuie generate

    *r=(unsigned int *)malloc(k*sizeof(unsigned int));
     if(*r==NULL)
  {
      printf("Nu s-a putut aloca memorie pentru vectorul r.");
      return;
  }

    x=r0;//se initializeaza valoarea initiala
    (*r)[0]=r0;

    for(i=1;i<=k;i++)
    {
        x=x^(x<<13);
        x=x^(x>>17);
        x=x^(x<<5);
        //se genereaza numerele aleatoare dupa modelul propus de George Marsaglia
        (*r)[i]=x;
    }
}

void permutare(unsigned int *r, unsigned int **perm, int latime, int inaltime)//genereaza o permutare aleatoare, folosind algoritmul Durstenfeld
{

    unsigned int i,k,x,aux;

    k=latime*inaltime;

    *perm=(unsigned int *)malloc(k*sizeof(unsigned int));
     if(*perm==NULL)
  {
      printf("Nu s-a putut aloca memorie pentru vectorul perm.");
      return;
  }

    for(i=0;i<k;i++)
        (*perm)[i]=i;//elementele vectorului sunt initializate cu pozitia respectiva

    for (i=k-1;i>=1;i--)
    {
        x=r[k-i]%(i+1);//calcularea pozitiei cu care se interschimba elementul de pe pozitia i, un numar cuprins intre 0 si i, generat aleator de functia XORSHIFT32

        aux=(*perm)[x];
        (*perm)[x]=(*perm)[i];
        (*perm)[i]=aux;
    }
}

void permutaPixeli(pixel *p, int latime, int inaltime, pixel **pp, unsigned int *perm)//permuta pixelii imaginii inițiale generand o imagine indermediara
{
    int i,k;

    k=latime*inaltime;

    *pp=(pixel *)malloc(sizeof(pixel)*k);
     if(*pp==NULL)
  {
      printf("Nu s-a putut aloca memorie pentru vectorul pp.");
      return;
  }

    for(i=0;i<k;i++)
        (*pp)[perm[i]]=p[i];
}

pixel xor_numar(pixel x, unsigned int a)//operatia XOR dintre un pixel si un numar intreg fara semn pe 32 de biti
{
    pixel y;
    Bytes t={a};//se determina cei 4 octeti ai numarului x

    y.blue=x.blue^t.b[0];
    y.green=x.green^t.b[1];
    y.red=x.red^t.b[2];

    return y;
}

pixel xor_pixeli(pixel x, pixel y)//operatia XOR aplicata pentru doi pixeli
{
    pixel z;

    z.blue=x.blue^y.blue;
    z.green=x.green^y.green;
    z.red=x.red^y.red;

    return z;
}

void criptare(char * imagine_initiala, char * imagine_criptata, char * cheie_secreta)//criptează o imagine de tip BMP
{
    int k,i;
    unsigned int *r,*perm,r0,sv;
    pixel *p, *pp,*c;

    FILE *fin=fopen(cheie_secreta,"r");
     if(fin==NULL)
 {
     printf("Fisier inexistent");
     return;
 }
 fscanf(fin,"%u%u",&r0,&sv);

    int latime, inaltime;
    dim(&latime,&inaltime,imagine_initiala);
    k=latime*inaltime;

    char *h;
    header(imagine_initiala,&h);

    liniarizare(imagine_initiala,&p,latime,inaltime);//se liniarizează imaginea inițială și se obține vectorul p
    XORSHIFT32(&r,r0,latime,inaltime);//se genereaza numerele aleatoare din vectorul r cu ajutorul funcției XORSHIFT32
    permutare(r,&perm,latime,inaltime);//se genereaza permutarea perm
    permutaPixeli(p,latime,inaltime,&pp,perm);//se permută pixelii formându-se imaginea intermediară stocată în vectorul pp

    c=(pixel *)malloc(sizeof(pixel)*k);
     if(c==NULL)
  {
      printf("Nu s-a putut aloca memorie pentru vectorul c.");
      return;
  }

    c[0]=xor_numar(pp[0],sv);
    c[0]=xor_numar(c[0],r[k]);

    for(i=1;i<k;i++)
    {
        c[i]=xor_pixeli(c[i-1],pp[i]);
        c[i]=xor_numar(c[i],r[k+i]);
    }

    deliniarizare(imagine_criptata,c,latime,inaltime,h);

    fclose(fin);
    free(p);
    free(h);
    free(r);
    free(perm);
    free(pp);
    free(c);
}

void inversa_permutarii(unsigned int *perm,unsigned int **perm_inv, int latime, int inaltime)//generează inversa permutării perm stocată în vectorul perm_inv
{
    int k,i;

    k=inaltime*latime;

    *perm_inv=(unsigned int *)malloc(k*sizeof(unsigned int));
     if(*perm_inv==NULL)
  {
      printf("Nu s-a putut aloca memorie pentru vectorul perm_inv.");
      return;
  }

    for(i=0;i<k;i++)
        (*perm_inv)[perm[i]]=i;
}

void substitutie(pixel **cp, pixel *c, unsigned int *r, unsigned int sv,int latime, int inaltime)//aplică fiecărui pixel din imaginea criptată c, inversa relației de substituție folosită în procesul de criptare, obținându-se o imagine intermediară cp
{
    int k,i;

    k=latime*inaltime;

    *cp=(pixel *)malloc(sizeof(pixel)*k);
     if(*cp==NULL)
  {
      printf("Nu s-a putut aloca memorie pentru vectorul cp.");
      return;
  }

    (*cp)[0]=xor_numar(c[0],sv);
    (*cp)[0]=xor_numar((*cp)[0],r[k]);

    for(i=1;i<k;i++)
    {
        (*cp)[i]=xor_pixeli(c[i-1],c[i]);
        (*cp)[i]=xor_numar((*cp)[i],r[k+i]);
    }
}

void decriptare(char * imagine_criptata, char * imagine_decriptata, char * cheie_secreta)//decriptează o imagine de tip BMP
{
    int k,i;
    unsigned int *r,*perm, *perm_inv,r0,sv;
    pixel *c,*cp, *d;

    FILE *fin=fopen(cheie_secreta,"r");
     if(fin==NULL)
 {
     printf("Fisier inexistent");
     return;
 }
 fscanf(fin,"%u%u",&r0,&sv);

    int latime, inaltime;
    dim(&latime,&inaltime,imagine_criptata);
    k=latime*inaltime;

    char *h;
    header(imagine_criptata,&h);

    liniarizare(imagine_criptata,&c,latime,inaltime);//se liniarizează imaginea criptată și se obține vectorul c
    XORSHIFT32(&r,r0,latime,inaltime);//se genereaza numerele aleatoare din vectorul r cu ajutorul funcției XORSHIFT32
    permutare(r,&perm,latime,inaltime);//se genereaza permutarea perm
    inversa_permutarii(perm, &perm_inv,latime,inaltime);//se determină inversa permutării perm salvată în vectorul perm_inv
    substitutie(&cp,c,r,sv,latime,inaltime);//se aplică funcția de substituție imaginii c, se obține imaginea intermediară cp

    d=(pixel *)malloc(sizeof(pixel)*k);
     if(d==NULL)
  {
      printf("Nu s-a putut aloca memorie pentru vectorul d.");
      return;
  }

    for(i=0;i<k;i++)
        d[perm_inv[i]]=cp[i];

    deliniarizare(imagine_decriptata,d,latime,inaltime,h);
    fclose(fin);

    free(h);
    free(r);
    free(c);
    free(perm);
    free(perm_inv);
    free(cp);
    free(d);

}

void chi_patrat(char *cale_imagine)//afiseaza valorile testului chi_patrat pentru o imagine BMP pe fiecare canal de culoare
{
    unsigned int *r,*g,*b;
    double xr=0,xb=0,xg=0,f_bar;
    int latime, inaltime,k;

    r=(unsigned int *)malloc(256*sizeof(unsigned int));
     if(r==NULL)
  {
      printf("Nu s-a putut aloca memorie pentru vectorul r.");
      return;
  }

    g=(unsigned int *)malloc(256*sizeof(unsigned int));
     if(g==NULL)
  {
      printf("Nu s-a putut aloca memorie pentru vectorul g.");
      return;
  }

    b=(unsigned int *)malloc(256*sizeof(unsigned int));
     if(b==NULL)
  {
      printf("Nu s-a putut aloca memorie pentru vectorul b.");
      return;
  }

    int i;
    for(i=0;i<=255;i++) //initializarea cu 0 a celor trei vectori de frecventa r,g,b utilizati calcularea frecvenței valorii i pe un canal de culoare al imaginii
    {
        r[i]=0;
        g[i]=0;
        b[i]=0;
    }

    pixel *p;

    dim(&latime,&inaltime, cale_imagine);
    liniarizare(cale_imagine,&p,latime, inaltime);
    k=latime*inaltime;

    for(i=0;i<k;i++)
    {
        r[p[i].red]++;
        g[p[i].green]++;
        b[p[i].blue]++;
    }

    f_bar=(latime*inaltime)/256.0; //frecvența estimată teoretic a oricărei valori i

    for(i=0;i<=255;i++)
    {
        xr=xr+(((r[i]-f_bar)*(r[i]-f_bar))/f_bar);//testul chi_patrat pentru canalul de culoare red
        xg=xg+(((g[i]-f_bar)*(g[i]-f_bar))/f_bar);//testul chi_patrat pentru canalul de culoare green
        xb=xb+(((b[i]-f_bar)*(b[i]-f_bar))/f_bar);//testul chi_patrat pentru canalul de culoare blue
    }

    printf("(%.2lf,%.2lf,%.2lf)\n",xr,xg,xb);
    free(r);
    free(g);
    free(b);
}

int main()
{
    char imagine_initiala[101], imagine_criptata[101], imagine_decriptata[101],cheia_secreta[101] ;
    printf("Numele fisierului care contine imaginea initiala: ");
    fgets(imagine_initiala, 101, stdin);
    imagine_initiala[strlen(imagine_initiala) - 1] = '\0';

    printf("Numele fisierului care contine imaginea criptata: ");
    fgets(imagine_criptata, 101, stdin);
    imagine_criptata[strlen(imagine_criptata) - 1] = '\0';

    printf("Numele fisierului care contine imaginea decriptata: ");
    fgets(imagine_decriptata, 101, stdin);
    imagine_decriptata[strlen(imagine_decriptata) - 1] = '\0';

    printf("Numele fisierului care contine cheia secreta: ");
    fgets(cheia_secreta, 101, stdin);
    cheia_secreta[strlen(cheia_secreta) - 1] = '\0';

    criptare(imagine_initiala,imagine_criptata,cheia_secreta);
    decriptare(imagine_criptata,imagine_decriptata,cheia_secreta);
    chi_patrat(imagine_initiala);
    chi_patrat(imagine_criptata);

    return 0;
}
