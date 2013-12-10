#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "opencv2/imgproc/imgproc_c.h"
#include "mosaic.h"

//---------------------------------------------------------------------------------
// Programa Principal
int main(int argc, char *argv[])
{
	Arq *banco;
	Hist *base;
	system("clear");

	if (argc < 4){
		puts("Uso :./mosaic <Operação> <nº de imagens/regioes> <IMAGEM_entrada> <IMAGEM_saida>");
//		if (argv[1] == "help"){
			puts(" Operação:\n	Caso seja 0, cria um novo index para o banco de dados, sendo seu uso da seguinte forma:");
			puts(" \tUso Cria index:./mosaic 0 <Diretorio_Banco_de_Dados> <Nº_Imagens>");
			puts(" 	Caso a operação seja 1, cria um Photomosaic, sendo deu uso da seguinte maneira:");
			puts(" \tUso Cria Photomosaic:./mosaic 1 <regioes> <Imagem_entrada> <Imagem_Saida>");
//		}
		return -1;
	}	

	if (atoi(argv[1])==0){	
		criarBanco(banco,argv);
		return 0;
	}
	else if (atoi(argv[1])==1){
		criaMosaic(banco,base,argv);
		return 0;
	}
	else{
		puts("O 1º argumento deve ser 0 ou 1 \n 0 -> Criar Index ( 0 <Dir_banco_de_dados> <nº de Imagens>)\n 1 -> Criar Photomosaic ( 1 <regioes> <Imagem_entrada> <Imagem_Saida>)");
	return -1;
	}
	
}

//---------------------------------------------------------------------------------
// Esta funcao cria uma base de imagens
int criarBanco(Arq *banco, char *argv[])
{
	IplImage *img;
	FILE *lista;
	int height,width,step,channels;
	uchar *data;
	int i,j,tam;
	char *nome;
	char *linhastr;

	//----Aloca Memoria--------------------------------------------
	banco = (Arq *)malloc(sizeof(Arq));
	banco->dir = (char *)malloc(TAM_STR*sizeof(char));
	nome = (char *)malloc(TAM_LIN*sizeof(char));
	linhastr = (char *)malloc(TAM_LIN*sizeof(char));
	//-------------------------------------------------------------

	banco->dir = argv[2];

	banco->num = atoi(argv[3]);

	//----Abre o Arquivo--------------------------------------
	if ((lista = fopen("base.txt","w+"))==NULL) 
	{
		printf("ERRO! A agenda nao pode ser aberta.\n");
		exit(0);
	}
	//---------------------------------------------------------

	for(i=1; i < ((banco->num) + 1); i++)	
	{
		sprintf(nome, "%simage%d.jpg", banco->dir, i); // Concatena o nome da imagem.
		// carrega imagem  
		img=cvLoadImage(nome,1);
		if(!img)
		{
			printf("Erro na leitura da imagem: %s\n",nome);
			exit(0);
		}

		// recupera parametros da imagem
		height    = img->height;
		width     = img->width;
		step      = img->widthStep;
		channels  = img->nChannels;
		data      = (uchar *)img->imageData;

		// converte para HSV e separa cada canal
		IplImage* imgHSV = cvCreateImage(cvGetSize(img), 8, 3);
		IplImage* imgH = cvCreateImage(cvGetSize(img), 8, 1);
		IplImage* imgS = cvCreateImage(cvGetSize(img), 8, 1);
		IplImage* imgV = cvCreateImage(cvGetSize(img), 8, 1);
		cvCvtColor(img,imgHSV,CV_BGR2HSV);
		cvSplit(imgHSV, imgH, imgS, imgV, NULL);

		// calcula histograma para cada canal e imprime
		int max_h,max_s,max_v,nb = 256;
		float range[] = {0, 255};
		float *ranges[] = { range };
		CvHistogram *hist = cvCreateHist(1, &nb, CV_HIST_ARRAY, ranges, 1);

		cvClearHist(hist);
		cvCalcHist(&imgH, hist, 0, 0);
		cvGetMinMaxHistValue( hist, NULL, NULL,NULL,&max_h);

		cvClearHist(hist);
		cvCalcHist(&imgS, hist, 0, 0);
		cvGetMinMaxHistValue( hist, NULL, NULL,NULL,&max_s);

		cvClearHist(hist);
		cvCalcHist(&imgV, hist, 0, 0);
		cvGetMinMaxHistValue( hist, NULL, NULL,NULL,&max_v);

		sprintf(linhastr, "%s %d %d %d\n", nome, max_h, max_s, max_v);
		printf("%s", linhastr);
 		
		fseek(lista, 0L, SEEK_END);
		fprintf(lista, "%s", linhastr);

		// libera a imagem
		cvReleaseImage(&img);
		cvReleaseImage(&imgHSV);
		cvReleaseImage(&imgH);
		cvReleaseImage(&imgS);
		cvReleaseImage(&imgV);
	}
	fclose(lista);
	free(banco);
	free(banco->dir);
	free(nome);
	free(linhastr);
	return 0;
}

//---------------------------------------------------------------------------------
// Cria uma Lista Encadeada
int criaMosaic(Arq *banco, Hist *base, char *argv[])
{
	Hist *imagem;
	IplImage *img; 
	IplImage *img_base;
	FILE *lista;
	int height,width,step,channels;
	uchar *data;
	int i,j,tam,dist,min_dist;
	char *minimo;
	char win1[] = "PHOTOMOSAIC";
	char win2[] = "TESTE";

	minimo = (char *)malloc(TAM_STR*sizeof(char));
	banco = (Arq *)malloc(sizeof(Arq));
	banco->img = (char *)malloc(TAM_STR*sizeof(char));
	banco->result = (char *)malloc(TAM_STR*sizeof(char));
	base = (Hist *)malloc(sizeof(Hist));
	base->foto = (char *)malloc(TAM_STR*sizeof(char));

	banco->reg = atoi(argv[2]);

	banco->img = argv[3];

	banco->result = argv[4];

	tam = banco->reg;

	// carrega imagem  
	img=cvLoadImage(banco->img,1);
	if(!img)
	{
		printf("Erro na leitura da imagem: %s\n",banco->img);
		exit(0);
	}

	// recupera parametros da imagem
	height    = img->height;
	width     = img->width;
	step      = img->widthStep;
	channels  = img->nChannels;
	data      = (uchar *)img->imageData;

	IplImage *img_final = cvCreateImage( cvSize((int)(width),(int)(height)),img->depth, channels );
	lista = fopen("base.txt","r");
	for(j=0;j<height-(height/tam);j+=height/tam)
		for(i=0;i<width-(width/tam);i+=width/tam) 
		{
			IplImage *img_colar = cvCreateImage( cvSize((int)(width/tam),(int)(height/tam)),img->depth, channels );
			cvSetImageROI(img, cvRect(i, j, width/tam, height/tam));
			imagem = analisaImagem(img);

		//----Abre o Arquivo--------------------------------------
			if (lista ==NULL) 
			{
				printf("ERRO! A agenda nao pode ser aberta.\n");
				exit(0);
			}
		//---------------------------------------------------------
			min_dist = 9999;
			while (!feof(lista)) {
				fscanf(lista, "%s %d %d %d", base->foto, &base->max_h, &base->max_s, &base->max_v);
				dist = calc(imagem, base);
				if (dist < min_dist){
					min_dist = dist;
					minimo = base->foto;
					img_base = cvLoadImage(minimo, 1);       
				}

			}


  
			if(!img_base) 
			{
				printf("Erro ao abrir a imagem: %s\n", minimo);
				exit(0);
			}

			cvResize(img_base, img_colar, CV_INTER_LINEAR);

			cvSetImageROI(img_final, cvRect(i, j, width/tam, height/tam));

			cvCopy(img_colar, img_final, NULL);

			cvReleaseImage(&img_base);
			cvReleaseImage(&img_colar);

			cvResetImageROI(img_final);
			cvResetImageROI(img);

			cvNamedWindow(win1, CV_WINDOW_NORMAL );
			cvShowImage(win1, img_final);

			cvWaitKey(1);

//			cvNamedWindow(win2, 1 );
//			cvShowImage(win2, img_final);


//			cvDestroyWindow(win1);
//			cvDestroyWindow(win2);
			cvSaveImage(banco->result,img_final,0);
			freopen("base.txt","r",lista);
		}
	
	cvWaitKey(100000);
	// salva a imagem de saida

	// libera a imagem
	cvReleaseImage(&img_final);
	cvReleaseImage(&img);
	return 0;
}

Hist *analisaImagem(IplImage *img){
        Hist *aux = (Hist *) malloc(sizeof(Hist));
                
        maxHSV(img, aux);
                
        return aux;
}

int calc(Hist *p, Hist *q)
{
        return sqrt( pow((p->max_h - q->max_h), 2) + pow((p->max_s - q->max_s), 2) + pow((p->max_v - q->max_v), 2) );
}

void maxHSV(IplImage *img, Hist *imagem)
{
        /* variaveis do histograma */
        int numBins = 256;
        float range[] = {0, 255};
        float *ranges[] = {range};
        
        /* cria um histograma */
        CvHistogram *hist = cvCreateHist(1, &numBins, CV_HIST_ARRAY, ranges, 1);
        cvClearHist(hist);
        
        /* aloca as imagens auxiliares */
        IplImage *imgHSV = cvCreateImage(cvGetSize(img), 8, 3);
        IplImage *imgH = cvCreateImage(cvGetSize(img), 8, 1);
        IplImage *imgS = cvCreateImage(cvGetSize(img), 8, 1);
        IplImage *imgV = cvCreateImage(cvGetSize(img), 8, 1);

        cvCvtColor(img, imgHSV, CV_BGR2HSV);                        // converte a img (RGB) para imgHSV (HSV)
        cvSplit(imgHSV, imgH, imgS, imgV, NULL);                // separa os canais da imgHSV
        
        cvCalcHist(&imgH, hist, 0, 0);                                        // calcula o histograma da imgH
        cvGetMinMaxHistValue(hist, NULL, NULL, NULL, &imagem->max_h); // coleta a cor dominante
        cvClearHist(hist);                                                                // reseta o histograma
        
        cvCalcHist(&imgS, hist, 0, 0);
        cvGetMinMaxHistValue(hist, NULL, NULL, NULL, &imagem->max_s);
        cvClearHist(hist);
        
        cvCalcHist(&imgV, hist, 0, 0);
        cvGetMinMaxHistValue(hist, NULL, NULL, NULL, &imagem->max_v);
        cvClearHist(hist);
        
        /* libera as imagens auxiliares */
        cvReleaseImage(&imgHSV);
        cvReleaseImage(&imgH);
        cvReleaseImage(&imgS);
        cvReleaseImage(&imgV);
}
