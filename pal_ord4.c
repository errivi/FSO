#include <stdio.h>		/* incloure definicions de funcions estandard */
#include <stdlib.h>
#include "winsuport2.h"		/* incloure definicions de funcions propies */
#include <stdint.h>
#include <pthread.h>
#include "memoria.h"
#include <stdbool.h>
#include "semafor.h"

int main(int n_args, char *ll_args[]){
  int f_h, i, ind, l_pal, fila, col;
  int id_v_pal, id_pal_ret, id_ipo_pf, *p_ipo_pf, id_ipo_pc, *p_ipo_pc, id_po_pf, id_mapa, id_final, id_moviments, *p_moviments, *p_mapa, sem_pant, sem_var, sem_pausa;
  char mov_desactivats = atoi(ll_args[9]);
  float *p_v_pal, *p_pal_ret, *p_po_pf;
  bool *p_final;
    
  if(n_args != 16){
  	fprintf(stderr, "proces pal_ord3 v_pal, pal_ret, ipo_pf, ipo_pc, po_pf, final, ind, l_pal, mapa, fila, columna, sem_pant, sem_var, sem_pausa\n");
  	exit(0);
  }

  id_v_pal = atoi(ll_args[1]);
  p_v_pal = map_mem(id_v_pal);

  id_pal_ret = atoi(ll_args[2]);
  p_pal_ret = map_mem(id_pal_ret);

  id_ipo_pf = atoi(ll_args[3]);
  p_ipo_pf = map_mem(id_ipo_pf);
  
  id_ipo_pc = atoi(ll_args[4]);
  p_ipo_pc = map_mem(id_ipo_pc);
  
  id_po_pf = atoi(ll_args[5]);
  p_po_pf = map_mem(id_po_pf);
  
  id_final = atoi(ll_args[6]);
  p_final = map_mem(id_final);  
  
  id_moviments = atoi(ll_args[12]);
  p_moviments = map_mem(id_moviments);
  ind = atoi(ll_args[7]);
  l_pal = atoi(ll_args[8]);

  id_mapa = atoi(ll_args[9]);
  p_mapa = map_mem(id_mapa);
  
  fila = atoi(ll_args[10]);
  col = atoi(ll_args[11]);
  win_set(p_mapa, fila, col);
  
  sem_pant = atoi(ll_args[13]);
  sem_var = atoi(ll_args[14]);
  sem_pausa = atoi(ll_args[15]);
  
  do{
  	
  	waitS(sem_pausa);
  	signalS(sem_pausa);
  	
  	f_h = p_po_pf[ind] + p_v_pal[ind];		/* posicio hipotetica de la paleta */
  	if (f_h != p_ipo_pf[ind])	/* si pos. hipotetica no coincideix amb pos. actual */
  	{
    		if (p_v_pal[ind] > 0.0)			/* verificar moviment cap avall */
    		{
			waitS(sem_pant);
			if (win_quincar(f_h+l_pal-1,p_ipo_pc[ind]) == ' ')   /* si no hi ha obstacle */
			{
	  			signalS(sem_pant);
	  			win_escricar(p_ipo_pf[ind],p_ipo_pc[ind],' ',NO_INV);      /* esborra primer bloc */
	  			p_po_pf[ind] += p_v_pal[ind]; p_ipo_pf[ind] = p_po_pf[ind];		/* actualitza posicio */
	  			waitS(sem_pant);
	  			win_escricar(p_ipo_pf[ind]+l_pal-1,p_ipo_pc[ind],'1',INVERS); /* impr. ultim bloc */
	  			signalS(sem_pant);
          			if (*p_moviments > 0) {
          				waitS(sem_var);
          				*p_moviments = *p_moviments -1;    /* he fet un moviment de la paleta */
          				signalS(sem_var);
          			}
			}
			else{		/* si hi ha obstacle, canvia el sentit del moviment */
	   			signalS(sem_pant);
	   			p_v_pal[ind] = -p_v_pal[ind];
	   			 	
	   		}
       		}
       		else			/* verificar moviment cap amunt */
       		{
			waitS(sem_pant);
			if (win_quincar(f_h,p_ipo_pc[ind]) == ' ')        /* si no hi ha obstacle */
			{
	  			
	  			win_escricar(p_ipo_pf[ind]+l_pal-1,p_ipo_pc[ind],' ',NO_INV); /* esbo. ultim bloc */
	  			signalS(sem_pant);
	  			p_po_pf[ind] += p_v_pal[ind]; p_ipo_pf[ind] = p_po_pf[ind];		/* actualitza posicio */
	  			waitS(sem_pant);
	  			win_escricar(p_ipo_pf[ind], p_ipo_pc[ind],'1',INVERS);	/* impr. primer bloc */
          			signalS(sem_pant);
          			if (*p_moviments > 0) {
          				waitS(sem_var);
          				*p_moviments = *p_moviments-1;    /* he fet un moviment de la paleta */
          				signalS(sem_var);
          			}
			}
			else{		/* si hi ha obstacle, canvia el sentit del moviment */
	   			signalS(sem_pant);
	   			p_v_pal[ind] = -p_v_pal[ind];
	   		}
	   		
      		}
      		
      }
      else p_po_pf[ind] += p_v_pal[ind]; 	/* actualitza posicio vertical real de la paleta */
      win_retard(p_pal_ret[ind]);
 }while(!*p_final);
 return 1;
}
