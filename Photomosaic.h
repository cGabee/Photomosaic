#define TAM_STR 100	/*Tamanho para string.*/
#define TAM_LIN 500	/*Tamanho para linha.*/

typedef struct arq {
	char *dir;
	char *img;
	char *result;
	int num, reg, max_h, max_s, max_v;
} Arq;

typedef struct hist {
	char *foto;
	int max_h, max_s, max_v;
} Hist;

int criarBanco(Arq *banco,char *argv[]);
int criaMosaic(Arq *banco, Hist *base, char *argv[]);
Hist *analisaImagem(IplImage *img);
int calc(Hist *p, Hist *q);
void maxHSV(IplImage *img, Hist *imagem);
