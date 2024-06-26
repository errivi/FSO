/*****************************************************************************/
/*									     */
/*				     Tennis3.c				     */
/*									     */
/*									     */
/*  Arguments del programa:						     */
/*     per controlar la posicio de tots els elements del joc, cal indicar    */
/*     el nom d'un fitxer de text que contindra la seguent informacio:	     */
/*		n_fil n_col m_por l_pal					     */
/*		pil_pf pil_pc pil_vf pil_vc pil_ret			     */
/*		ipo_pf ipo_pc po_vf pal_ret				     */
/*									     */
/*     on 'n_fil', 'n_col' son les dimensions del taulell de joc, 'm_por'    */
/*     es la mida de les dues porteries, 'l_pal' es la longitud de les dues  */
/*     paletes; 'pil_pf', 'pil_pc' es la posicio inicial (fila,columna) de   */
/*     la pilota, mentre que 'pil_vf', 'pil_vc' es la velocitat inicial,     */
/*     pil_ret es el percentatge respecte al retard passat per paràmetre;    */
/*     finalment, 'ipo_pf', 'ipo_pc' indicara la posicio del primer caracter */
/*     de la paleta de l'ordinador, mentre que la seva velocitat vertical    */
/*     ve determinada pel parametre 'po_fv', i pal_ret el percentatge de     */
/*     retard en el moviment de la paleta de l'ordinador.		     */
/*									     */
/*     A mes, es podra afegir un segon argument opcional per indicar el      */
/*     retard de moviment de la pilota i la paleta de l'ordinador (en ms);   */
/*     el valor d'aquest parametre per defecte es 100 (1 decima de segon).   */
/*									     */
/*  Compilar i executar:					  	     */
/*     El programa invoca les funcions definides en 'winsuport.o', les       */
/*     quals proporcionen una interficie senzilla per a crear una finestra   */
/*     de text on es poden imprimir caracters en posicions especifiques de   */
/*     la pantalla (basada en CURSES); per tant, el programa necessita ser   */
/*     compilat amb la llibreria 'curses':				     */
/*									     */
/*	   $ gcc tennis2.c winsuport.o -o tennis2 -lcurses		     */
/*	   $ tennis2 fit_param [retard]					     */
/*									     */
/*  Codis de retorn:						  	     */
/*     El programa retorna algun dels seguents codis al SO:		     */
/*	0  ==>  funcionament normal					     */
/*	1  ==>  numero d'arguments incorrecte 				     */
/*	2  ==>  fitxer no accessible					     */
/*	3  ==>  dimensions del taulell incorrectes			     */
/*	4  ==>  parametres de la pilota incorrectes			     */
/*	5  ==>  parametres d'alguna de les paletes incorrectes		     */
/*	6  ==>  no s'ha pogut crear el camp de joc (no pot iniciar CURSES)   */
/*****************************************************************************/




#include <stdio.h>		/* incloure definicions de funcions estandard */
#include <stdlib.h>
#include "winsuport2.h"		/* incloure definicions de funcions propies */
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include "memoria.h"
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "semafor.h"

#define MIN_FIL 7		/* definir limits de variables globals */
#define MAX_FIL 25
#define MIN_COL 10
#define MAX_COL 80
#define MIN_PAL 3
#define MIN_VEL -1.0
#define MAX_VEL 1.0
#define MIN_RET 0.0
#define MAX_RET 5.0
#define MAX_PAL_ORD 9
#define MAX_THREADS 11

/* variables globals */
pthread_t tid[MAX_THREADS];     /* taula amb identificadors dels threads */
pid_t tpid[MAX_PAL_ORD];		/*taula amb identificador dels processos fills*/
int n_fil, n_col, m_por, id_mapa, *p_mapa, mida_mapa;	/* dimensions del taulell i porteries */
int l_pal;			/* longitud de les paletes */
float v_pal[MAX_PAL_ORD];			/* velocitat de la paleta del programa */
float pal_ret[MAX_PAL_ORD];			/* percentatge de retard de la paleta */

int ipu_pf, ipu_pc;      	/* posicio del la paleta d'usuari */
int ipo_pf[MAX_PAL_ORD], ipo_pc[MAX_PAL_ORD];      	/* posicio del la paleta de l'ordinador */
float po_pf[MAX_PAL_ORD];	/* pos. vertical de la paleta de l'ordinador, en valor real */

int ipil_pf, ipil_pc;		/* posicio de la pilota, en valor enter */
float pil_pf, pil_pc;		/* posicio de la pilota, en valor real */
float pil_vf, pil_vc;		/* velocitat de la pilota, en valor real*/
float pil_ret;			/* percentatge de retard de la pilota */

int retard;		/* valor del retard de moviment, en mil.lisegons */
int moviments, mov_actuals, mov_totals, *p_moviments;		/* numero max de moviments paletes per acabar el joc */
int pal_ord_actuals=0;

int tec, cont;
char mapa[20];
bool pausa;
int mov_desactivats;
int segons=0;
int minuts=0;

pthread_mutex_t mutex_pant= PTHREAD_MUTEX_INITIALIZER;//inicialitzem llavors dels semafors
pthread_mutex_t mutex_var= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_pausa= PTHREAD_MUTEX_INITIALIZER;
int sem_pant, sem_var, sem_pausa;

/* funcio per realitzar la carrega dels parametres de joc emmagatzemats */
/* dins un fitxer de text, el nom del qual es passa per referencia en   */
/* 'nom_fit'; si es detecta algun problema, la funcio avorta l'execucio */
/* enviant un missatge per la sortida d'error i retornant el codi per-	*/
/* tinent al SO (segons comentaris del principi del programa).		*/
void carrega_parametres(const char *nom_fit)
{
  FILE *fit;

  fit = fopen(nom_fit,"rt");		/* intenta obrir fitxer */
  if (fit == NULL)
  {	fprintf(stderr,"No s'ha pogut obrir el fitxer \'%s\'\n",nom_fit);
  	exit(2);
  }

  if (!feof(fit)) fscanf(fit,"%d %d %d %d\n",&n_fil,&n_col,&m_por,&l_pal);
  if ((n_fil < MIN_FIL) || (n_fil > MAX_FIL) ||
	(n_col < MIN_COL) || (n_col > MAX_COL) || (m_por < 0) ||
	 (m_por > n_fil-3) || (l_pal < MIN_PAL) || (l_pal > n_fil-3))
  {
	fprintf(stderr,"Error: dimensions del camp de joc incorrectes:\n");
	fprintf(stderr,"\t%d =< n_fil (%d) =< %d\n",MIN_FIL,n_fil,MAX_FIL);
	fprintf(stderr,"\t%d =< n_col (%d) =< %d\n",MIN_COL,n_col,MAX_COL);
	fprintf(stderr,"\t0 =< m_por (%d) =< n_fil-3 (%d)\n",m_por,(n_fil-3));
	fprintf(stderr,"\t%d =< l_pal (%d) =< n_fil-3 (%d)\n",MIN_PAL,l_pal,(n_fil-3));
	fclose(fit);
	exit(3);
  }

  if (!feof(fit)) fscanf(fit,"%d %d %f %f %f\n",&ipil_pf,&ipil_pc,&pil_vf,&pil_vc,&pil_ret);
  if ((ipil_pf < 1) || (ipil_pf > n_fil-3) ||
	(ipil_pc < 1) || (ipil_pc > n_col-2) ||
	(pil_vf < MIN_VEL) || (pil_vf > MAX_VEL) ||
	(pil_vc < MIN_VEL) || (pil_vc > MAX_VEL) ||
	(pil_ret < MIN_RET) || (pil_ret > MAX_RET))
  {
	fprintf(stderr,"Error: parametre pilota incorrectes:\n");
	fprintf(stderr,"\t1 =< ipil_pf (%d) =< n_fil-3 (%d)\n",ipil_pf,(n_fil-3));
	fprintf(stderr,"\t1 =< ipil_pc (%d) =< n_col-2 (%d)\n",ipil_pc,(n_col-2));
	fprintf(stderr,"\t%.1f =< pil_vf (%.1f) =< %.1f\n",MIN_VEL,pil_vf,MAX_VEL);
	fprintf(stderr,"\t%.1f =< pil_vc (%.1f) =< %.1f\n",MIN_VEL,pil_vc,MAX_VEL);
	fprintf(stderr,"\t%.1f =< pil_ret (%.1f) =< %.1f\n",MIN_RET,pil_ret,MAX_RET);
	fclose(fit);
	exit(4);
  }

  
  while(!feof(fit) && pal_ord_actuals<MAX_PAL_ORD){
  	if (!feof(fit)) fscanf(fit,"%d %d %f %f\n",&ipo_pf[pal_ord_actuals],&ipo_pc[pal_ord_actuals],&v_pal[pal_ord_actuals],&pal_ret[pal_ord_actuals]);
  	if ((ipo_pf[pal_ord_actuals] < 1) || (ipo_pf[pal_ord_actuals]+l_pal > n_fil-2) ||
		(ipo_pc[pal_ord_actuals] < 5) || (ipo_pc[pal_ord_actuals] > n_col-2) ||
		(v_pal[pal_ord_actuals] < MIN_VEL) || (v_pal[pal_ord_actuals] > MAX_VEL) ||
		(pal_ret[pal_ord_actuals] < MIN_RET) || (pal_ret[pal_ord_actuals] > MAX_RET))
	    {
		fprintf(stderr,"Error: parametres paleta ordinador incorrectes:\n");
		fprintf(stderr,"\t1 =< ipo_pf (%d) =< n_fil-l_pal-3 (%d)\n",ipo_pf[pal_ord_actuals],(n_fil-l_pal-3));
		fprintf(stderr,"\t5 =< ipo_pc (%d) =< n_col-2 (%d)\n",ipo_pc[pal_ord_actuals],(n_col-2));
		fprintf(stderr,"\t%.1f =< v_pal (%.1f) =< %.1f\n",MIN_VEL,v_pal[pal_ord_actuals],MAX_VEL);
		fprintf(stderr,"\t%.1f =< pal_ret (%.1f) =< %.1f\n",MIN_RET,pal_ret[pal_ord_actuals],MAX_RET);
		fclose(fit);
		exit(5);
	    }
	   
	
	pal_ord_actuals++;
	
  }
  fprintf(stderr, "%d\n", pal_ord_actuals);
  fclose(fit);			/* fitxer carregat: tot OK! */
}


/* funcio per inicialitar les variables i visualitzar l'estat inicial del joc */
int inicialitza_joc(void)
{
  int i, i_port, f_port, retwin;
  char strin[51];

  retwin = win_ini(&n_fil,&n_col,'+',INVERS);   /* intenta crear taulell */
  fprintf(stderr, "%d\n", retwin);
  id_mapa = ini_mem(retwin); //creem la zona de memoria del mapa
  p_mapa = map_mem(id_mapa);
  sprintf(mapa, "%i", id_mapa);
  fprintf(stderr, "punter a mapa: %d, id del mapa: %d\n", p_mapa, id_mapa);
  fprintf(stderr, "fila: %d, col:%d\n", n_fil, n_col);
  win_set(p_mapa, n_fil, n_col);
  
  if (retwin < 0)       /* si no pot crear l'entorn de joc amb les curses */
  { fprintf(stderr,"Error en la creacio del taulell de joc:\t");
    switch (retwin)
    {   case -1: fprintf(stderr,"camp de joc ja creat!\n");
                 break;
        case -2: fprintf(stderr,"no s'ha pogut inicialitzar l'entorn de curses!\n");
 		 break;
        case -3: fprintf(stderr,"les mides del camp demanades son massa grans!\n");
                 break;
        case -4: fprintf(stderr,"no s'ha pogut crear la finestra!\n");
                 break;
     }
     return(retwin);
  }
  fprintf(stderr, "crea porteries\n");
  i_port = n_fil/2 - m_por/2;	    /* crea els forats de la porteria */
  if (n_fil%2 == 0) i_port--;
  if (i_port == 0) i_port=1;
  f_port = i_port + m_por -1;
  for (i = i_port; i <= f_port; i++)
  {	win_escricar(i,0,' ',NO_INV);
	win_escricar(i,n_col-1,' ',NO_INV);
  }
  

  ipu_pf = n_fil/2; ipu_pc = 3;		/* inicialitzar pos. paletes */
  if (ipu_pf+l_pal >= n_fil-3) ipu_pf = 1;
  for (int h=0; h<MAX_PAL_ORD; h++){
  	for (int j=0; j< l_pal; j++){
  		win_escricar(ipo_pf[h] +j, ipo_pc[h], '0',INVERS);
  	}
  	po_pf[h] = ipo_pf[h];		/* fixar valor real paleta ordinador */
  }
  
  for (int j=0; j< l_pal; j++)	    /* dibuixar paleta inicialment */
  {	
  	win_escricar(ipu_pf +j, ipu_pc, '0',INVERS);
  }
  	

  pil_pf = ipil_pf; pil_pc = ipil_pc;	/* fixar valor real posicio pilota */
  win_escricar(ipil_pf, ipil_pc, '.',INVERS);	/* dibuix inicial pilota */
  
  sprintf(strin,"Tecles: \'%c\'-> amunt, \'%c\'-> avall, RETURN-> sortir.",
		TEC_AMUNT, TEC_AVALL);
  win_escristr(strin);
  win_update();
  return(retwin);
}


/* funcio per moure la pilota; retorna un valor amb alguna d'aquestes	*/
/* possibilitats:							*/
/*	-1 ==> la pilota no ha sortit del taulell			*/
/*	 0 ==> la pilota ha sortit per la porteria esquerra		*/
/*	>0 ==> la pilota ha sortit per la porteria dreta		*/
void * moure_pilota(void * cap)
{
  int f_h, c_h, i;
  char rh,rv,rd,pd;
  do{
	//pthread_mutex_lock(&mutex_pausa);  //bloqueig del thread si es prem espai
	//pthread_mutex_unlock(&mutex_pausa); 
	waitS(sem_pausa);
	signalS(sem_pausa);
  	
	  	
	  f_h = pil_pf + pil_vf;		/* posicio hipotetica de la pilota */
	  c_h = pil_pc + pil_vc;
	  //pthread_mutex_lock(&mutex_var);
	  waitS(sem_var);
	  cont = -1;		/* inicialment suposem que la pilota no surt */
	  signalS(sem_var);
	  //pthread_mutex_unlock(&mutex_var);
	  rh = rv = rd = pd = ' ';
	  if ((f_h != ipil_pf) || (c_h != ipil_pc))
	  {		/* si posicio hipotetica no coincideix amb la pos. actual */
	    if (f_h != ipil_pf)		/* provar rebot vertical */
	    {	
	    	//pthread_mutex_lock(&mutex_pant);
	    	waitS(sem_pant);
	    	rv = win_quincar(f_h,ipil_pc);	/* veure si hi ha algun obstacle */
	    	signalS(sem_pant);
	    	//pthread_mutex_unlock(&mutex_pant);
		if (rv != ' ')			/* si no hi ha res */
		{   pil_vf = -pil_vf;		/* canvia velocitat vertical */
		    f_h = pil_pf+pil_vf;	/* actualitza posicio hipotetica */
		}
	    }
	    if (c_h != ipil_pc)		/* provar rebot horitzontal */
	    {	
	    	//pthread_mutex_lock(&mutex_pant);
	    	waitS(sem_pant);
	    	rh = win_quincar(ipil_pf,c_h);	/* veure si hi ha algun obstacle */
	    	signalS(sem_pant);
	    	//pthread_mutex_unlock(&mutex_pant);
		if (rh != ' ')			/* si no hi ha res */
		{    pil_vc = -pil_vc;		/* canvia velocitat horitzontal */
		     c_h = pil_pc+pil_vc;	/* actualitza posicio hipotetica */
		}
	    }
	    if ((f_h != ipil_pf) && (c_h != ipil_pc))	/* provar rebot diagonal */
	    {	
	    	//pthread_mutex_lock(&mutex_pant);
	    	waitS(sem_pant);
	    	rd = win_quincar(f_h,c_h);
	    	signalS(sem_pant);
	    	//pthread_mutex_unlock(&mutex_pant);
		if (rd != ' ')				/* si no hi ha obstacle */
		{    pil_vf = -pil_vf; pil_vc = -pil_vc;	/* canvia velocitats */
		     f_h = pil_pf+pil_vf;
		     c_h = pil_pc+pil_vc;		/* actualitza posicio entera */
		}
    	    }
    		//pthread_mutex_lock(&mutex_pant);
    		waitS(sem_pant);
    		if (win_quincar(f_h,c_h) == ' ')	/* verificar posicio definitiva */
    		{						/* si no hi ha obstacle */
    			signalS(sem_pant);
    			//pthread_mutex_unlock(&mutex_pant);
			//pthread_mutex_lock(&mutex_pant);
			waitS(sem_pant);
			win_escricar(ipil_pf,ipil_pc,' ',NO_INV);	/* esborra pilota */
			signalS(sem_pant);
			//pthread_mutex_unlock(&mutex_pant);
			pil_pf += pil_vf; pil_pc += pil_vc;
			ipil_pf = f_h; ipil_pc = c_h;		/* actualitza posicio actual */
			
			if ((ipil_pc > 0) && (ipil_pc <= n_col)){	/* si no surt */
				//pthread_mutex_lock(&mutex_pant);
				waitS(sem_pant);
				win_escricar(ipil_pf,ipil_pc,'.',INVERS); /* imprimeix pilota */
				signalS(sem_pant);
				//pthread_mutex_unlock(&mutex_pant);
				}
			else{
				//pthread_mutex_lock(&mutex_var);
				waitS(sem_var);
				cont = ipil_pc;	/* codi de finalitzacio de partida */
				signalS(sem_var);
				//pthread_mutex_unlock(&mutex_var);
			}
    		}
    		else{
    		    signalS(sem_pant);
    	            //pthread_mutex_unlock(&mutex_pant);
    		} 
    		
  	  }
  	else { pil_pf += pil_vf; pil_pc += pil_vc; }
  	win_retard(retard);
  }while((tec != TEC_RETURN) && (cont==-1) && ((*p_moviments > 0) || mov_desactivats==1));
  i=1;
  return((void *) (intptr_t) i);
}


/* funcio per moure la paleta de l'usuari en funcio de la tecla premuda */
void * mou_paleta_usuari(void * cap)
{
  int i;
  do{
  	
  	if(tec==TEC_ESPAI){ //control de bloqueig de fil
  		if(!pausa){
  			//pthread_mutex_lock(&mutex_pausa);
  			waitS(sem_pausa);
  			pausa=true;
  		}
  		else{
  			//pthread_mutex_unlock(&mutex_pausa);
  			signalS(sem_pausa);
  			pausa=false;
  		}
  	}
  	
  	//pthread_mutex_lock(&mutex_pant);
  	waitS(sem_pant);
  	tec=win_gettec();
  	signalS(sem_pant);
  	//pthread_mutex_unlock(&mutex_pant);
  	
  	
  	
  	if(!pausa){
  		if ((tec == TEC_AVALL) && (win_quincar(ipu_pf+l_pal,ipu_pc) == ' '))
  		{
    			//pthread_mutex_lock(&mutex_pant);
    			waitS(sem_pant);
    			win_escricar(ipu_pf,ipu_pc,' ',NO_INV);	   /* esborra primer bloc */
    			signalS(sem_pant);
    			//pthread_mutex_unlock(&mutex_pant);//prueba, si no borrar
    			ipu_pf++;					   /* actualitza posicio */
    			//pthread_mutex_lock(&mutex_pant);
    			waitS(sem_pant);
    			win_escricar(ipu_pf+l_pal-1,ipu_pc,'0',INVERS); /* impri. ultim bloc */
    			signalS(sem_pant);
    			//pthread_mutex_unlock(&mutex_pant);//prueba, si no borrar
    			if (*p_moviments > 0){
    				//pthread_mutex_lock(&mutex_var);
    				waitS(sem_var);
    				*p_moviments = *p_moviments-1;    /* he fet un moviment de la paleta */
    				signalS(sem_var);
    				//pthread_mutex_unlock(&mutex_var);
    			}
  		}
  		if ((tec == TEC_AMUNT) && (win_quincar(ipu_pf-1,ipu_pc) == ' '))
  		{
  			//pthread_mutex_lock(&mutex_pant);
  			waitS(sem_pant);
    			win_escricar(ipu_pf+l_pal-1,ipu_pc,' ',NO_INV); /* esborra ultim bloc */
    			signalS(sem_pant);
    			//pthread_mutex_unlock(&mutex_pant);
    			ipu_pf--;					    /* actualitza posicio */
    			//pthread_mutex_lock(&mutex_pant);
    			waitS(sem_pant);
    			win_escricar(ipu_pf,ipu_pc,'0',INVERS);	    /* imprimeix primer bloc */
    			signalS(sem_pant);
    			//pthread_mutex_unlock(&mutex_pant);
    			if (*p_moviments > 0){
    				//pthread_mutex_lock(&mutex_var);
    				waitS(sem_var);
    				*p_moviments = *p_moviments-1;    /* he fet un moviment de la paleta */
    				signalS(sem_var);
    				//pthread_mutex_unlock(&mutex_var);
    			}
  		}
  	}
  	
  	win_retard(retard);
  	
  }while((tec != TEC_RETURN) && (cont==-1) && ((*p_moviments > 0) || mov_desactivats==1));
  i=1;
  return((void *) (intptr_t) i);
}

void * comptadors(void* cap){
  
  char string[50];
 do{				/********** bucle tasca independent **********/	
  		
  	
  	mov_actuals = mov_totals-*p_moviments;
  	if (!mov_desactivats) sprintf(string, "Temps de joc:%d:%d Mov_res/Mov_act:%d/%d", minuts, segons, moviments, mov_actuals);
  	else sprintf(string, "Temps de joc: %d:%d", minuts, segons);
  	
  	//pthread_mutex_lock(&mutex_pant);
  	waitS(sem_pant);
  	win_escristr(string);	
  	signalS(sem_pant);
  	//pthread_mutex_unlock(&mutex_pant);
  	segons++;
  	if (segons==60){
  		segons=0;
  		minuts++;
  	}
  	
  	
  	win_retard(1000);
  	
  }while ((tec != TEC_RETURN) && (cont==-1) && ((*p_moviments > 0) || mov_desactivats==1));
  return((void *) (intptr_t)1);
}
/* programa principal*/
int main(int n_args, const char *ll_args[])
{
  		/* variables locals */
  int id_v_pal, id_pal_ret, id_ipo_pf, *p_ipo_pf, id_ipo_pc, *p_ipo_pc, id_po_pf, id_moviments /*id_mapa, *p_mapa, mida_mapa*/;
  float *p_v_pal, *p_pal_ret, *p_po_pf;
  int id_final;
  char a1[20], a2[20], a3[20], a4[20], a5[20], a6[20], a7[20], a8[20], fila[20], col[20], mov[20], pant[20], var[20], pausa[20];
  bool final=false, *p_final;
  
  if ((n_args != 3) && (n_args !=4))
  {	fprintf(stderr,"Comanda: tennis0 fit_param moviments [retard]\n");
  	exit(1);
  }
  carrega_parametres(ll_args[1]);
  moviments=atoi(ll_args[2]);

  if (n_args == 4) retard = atoi(ll_args[3]);
  else retard = 100;
  mida_mapa=inicialitza_joc();  
  id_mapa = ini_mem(mida_mapa); //creem la zona de memoria del mapa
  p_mapa = map_mem(id_mapa);
    sprintf(mapa, "%i", id_mapa);
    fprintf(stderr, "punter a mapa: %d, id del mapa: %d\n", p_mapa, id_mapa);
  fprintf(stderr, "fila: %d, col:%d\n", n_fil, n_col);
  win_set(p_mapa, n_fil, n_col);

  if (mida_mapa<=0)    /* intenta crear el taulell de joc */
     exit(4);   /* aborta si hi ha algun problema amb taulell */
 
  id_v_pal = ini_mem(sizeof(float)*MAX_PAL_ORD);
  p_v_pal = (float *)map_mem(id_v_pal);
  memcpy(p_v_pal, v_pal, sizeof(float) * MAX_PAL_ORD); 
  sprintf(a1, "%i", id_v_pal);
  
  id_pal_ret = ini_mem(sizeof(float)*MAX_PAL_ORD);
  p_pal_ret = map_mem(id_pal_ret);
  memcpy(p_pal_ret, pal_ret, sizeof(float) * MAX_PAL_ORD); 
  sprintf(a2, "%i", id_pal_ret);
  
  id_ipo_pf = ini_mem(sizeof(int)*MAX_PAL_ORD);
  p_ipo_pf = map_mem(id_ipo_pf);
  memcpy(p_ipo_pf, ipo_pf, sizeof(int) * MAX_PAL_ORD); 
  sprintf(a3, "%i", id_ipo_pf);
  
  id_ipo_pc = ini_mem(sizeof(int)*MAX_PAL_ORD);
  p_ipo_pc = map_mem(id_ipo_pc);
  memcpy(p_ipo_pc, ipo_pc, sizeof(int) * MAX_PAL_ORD); 
  sprintf(a4, "%i", id_ipo_pc);
  
  id_po_pf = ini_mem(sizeof(float)*MAX_PAL_ORD);
  p_po_pf = map_mem(id_po_pf);
  memcpy(p_po_pf, po_pf, sizeof(float) * MAX_PAL_ORD); 
  sprintf(a5, "%i", id_po_pf);
  
  sprintf(a8, "%i", l_pal);
  
  sprintf(fila, "%i", n_fil);
  sprintf(col, "%i", n_col);
  
  id_final = ini_mem(sizeof(bool));
  p_final = map_mem(id_final);
  sprintf(a6, "%i", id_final);
  
  id_moviments = ini_mem(sizeof(int));
  p_moviments=map_mem(id_moviments);
  *p_moviments = moviments;
  sprintf(mov, "%i", id_moviments);
  
  if(moviments==0) mov_desactivats=1;
  else mov_desactivats=0;
  
  mov_totals=moviments;
  
  /*pthread_mutex_init(&mutex_pant, NULL);
  pthread_mutex_init(&mutex_var, NULL);
  pthread_mutex_init(&mutex_pausa, NULL);*/
  
  sem_pant = ini_sem(1);
  sem_var = ini_sem(1);
  sem_pausa = ini_sem(1);
  sprintf(pant, "%i", sem_pant);
  sprintf(var, "%i", sem_var);
  sprintf(pausa, "%i", sem_pausa);
  
  if(pthread_create(&tid[0],NULL,moure_pilota,(void *) NULL)!=0){
  	printf("Error al crear el fil de la pilota\n");
  }
  if(pthread_create(&tid[1],NULL,mou_paleta_usuari, (void *) NULL)!=0){
  	printf("Error al crear el fil de la paleta del usuari\n");
  }
  
  if(pthread_create(&tid[2],NULL, comptadors, (void *) NULL)!=0){
  	printf("Error al crear el fil dels comptadors\n");
  }
  int n=3;
  fprintf(stderr, "Paletas ordenador: %d\n", pal_ord_actuals);
  for (int i=0; i<pal_ord_actuals; i++){
  
  	p_pal_ret[i] = retard * (p_pal_ret[i]);
  	
  	tpid[i] = fork();
  	fprintf(stderr, "fork a devuelto: %d\n", (int)tpid[i]);
  	fprintf(stderr, "indice actual: %d\n", i);
  	if(tpid[i] == (pid_t)0){
  		fprintf(stderr, "ha entrado if del exec\n");
  		sprintf(a7, "%i", i);
  		execlp("./pal_ord4", "pal_ord4", a1, a2, a3, a4, a5, a6, a7, a8, mapa, fila, col, mov, pant, var, pausa, (char *)0);
  		fprintf(stderr, "error: no puc executar el proces fill \'pal_ord3\'\n");
  		exit(0);
  	}
  	else if (tpid[i] < (pid_t)0) {
        		fprintf(stderr, "Error al crear el proces fill %d.\n", i);
        	}
  }
  
  do{				/********** bucle update **********/	
  	//pthread_mutex_lock(&mutex_pant);
  	waitS(sem_pant);
  	win_update();
  	signalS(sem_pant);
  	//pthread_mutex_unlock(&mutex_pant);
  	win_retard(50);
 
  }while ((tec != TEC_RETURN) && (cont==-1) && ((*p_moviments > 0) || mov_desactivats==1));
  *p_final=true;
  		
  int fils=0;
  int f;
  
  for(int i=0; i<n; i++){
  pthread_join(tid[i], (void **)&f);
  fils+=f;
  }
  /*fprintf(stderr, "eliminando mutex_pant\n");
  pthread_mutex_destroy(&mutex_pant);
    fprintf(stderr, "eliminando mutex_var\n");
  pthread_mutex_destroy(&mutex_var);
    fprintf(stderr, "eliminando mutex_pausa\n");
  pthread_mutex_destroy(&mutex_pausa);*/
  
  int processos;
  int processos_total = 0;
  int codigo_fin;
  for(int i=0; i<pal_ord_actuals; i++){
  	fprintf(stderr, "Esperando que acabe el proceso %d\n", (int)tpid[i]);
  	codigo_fin=waitpid(tpid[i], &processos, 0);
  	fprintf(stderr, "Ha acabado con codigo: %d\n", codigo_fin);
  	processos = processos >> 8;
  	processos_total+=processos; 
  }
  
  elim_sem(sem_pant);
  elim_sem(sem_var);
  elim_sem(sem_pausa);
  
  fprintf(stderr, "eliminando id_v_pal\n");
  elim_mem(id_v_pal);
  fprintf(stderr, "eliminando id_pal_ret\n");
  elim_mem(id_pal_ret);
  fprintf(stderr, "eliminando id_ipo_pf\n");
  elim_mem(id_ipo_pf);
  fprintf(stderr, "eliminando id_ipo_pc\n");
  elim_mem(id_ipo_pc);
  fprintf(stderr, "eliminando id_po_pf\n");
  elim_mem(id_po_pf);
  fprintf(stderr, "eliminando id_moviments\n");
  elim_mem(id_moviments);
  fprintf(stderr, "eliminando id_mapa\n");
  //elim_mem(id_tec);
  //elim_mem(id_cont);
  elim_mem(id_mapa);
  fprintf(stderr, "eliminando id_final\n");
  elim_mem(id_final);

  win_fi();
  printf("S'han executat %d processos\n", processos_total);
  printf("S'han executat %d fils de un total de %d fils creats\n", fils, n);
  if (tec == TEC_RETURN) printf("S'ha aturat el joc amb la tecla RETURN!\n");
  else { if (cont == 0 || (*p_moviments == 0 && mov_desactivats==0)) printf("Ha guanyat l'ordinador!\n");
         else printf("Ha guanyat l'usuari!\n"); }
  printf("El joc ha durat %d:%d \n", minuts, segons);
  return(0);
}
