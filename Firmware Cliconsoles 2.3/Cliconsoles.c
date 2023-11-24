//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//O projeto CLICONSOLES (Climatização e Controle do Solo em Estufa), foi desenvolvido por:                                                     //
//Henrique Xavier da Costa;                                                                                                                   //                                                                                                                                     
//Com a finalidade de controlar os fatores Temperatura, Umidade do Ar (RHar) e Umidade do Solo (RHsolo) através de acionamentos basicos de   //
//um aquecedor, um cooler, uma bomba d'agua e um pulverizador. */                                                                           // 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <18F452.H>
#device ADC=10
#use delay (clock=16000000)
#fuses HS, NOWDT, PUT, NOBROWNOUT, NOLVP, PROTECT

#use rs232(baud=9600,parity=N,xmit=PIN_C6,rcv=PIN_C7,bits=8)// Pino RC6 é Tx e o Pino Rx é o Pino RC7

#include <mod_lcd.c>  // Biblioteca retirada do livro Linguagem C para PIC de Fabio Pereira

//.....................................................................................................  
int comando=0;
  
#int_rda
void isr() 
{
   char  dado;

   dado = getch();
   if (dado=='v'||dado=='V')
   comando=1;
   if (dado=='A')
   comando=2;
   if (dado=='a')
   comando=3;
   if (dado=='R')
   comando=4;
   if (dado=='r')
   comando=5;
   if (dado=='U')
   comando=6;
   if (dado=='u')
   comando=7;
   if (dado=='I')
   comando=8;
   if (dado=='i')
   comando=9;
   if (dado=='M'||dado=='m')
   comando=10;
   if (dado=='E'||dado=='e')
   comando=11;
}
//...................................................................................................   
   
void main(void)
{

   
   enable_interrupts(int_rda);
   enable_interrupts(global);
   
 //VARIAVEIS PARA CONTROLE DE TEMPERATURA (Sensor: LM35)
   float Temperatura=0;                   //Temperatura final medida
   int16 mediaADtpt;                      //Media do conversor AD para leitura de temperatura
   int   Tmin=0, Tmax=25;                 //Temperatura minima e maxima (Parametrizavel)
   int   Temperaturaminimaregistrada=99;  //Registrador de valor maximo lido de Temperatura
   int   Temperaturamaximaregistrada=0;   //Registrador de valor minimo lido de Temperatura
   int1  Mododoaquecedor=0;               //Variavel para controlar o modo do aquecedor (Ligado=1/Automatico=0)
   int1  Mododoresfriador=0;              //Variavel para controlar o modo do resfriador (Ligado=1/Automatico=0)
   int1  aquec_aut=0;
   int1  resfr_aut=0;
   
   //VARIAVEIS PARA CONTROLE DE UMIDADE DO SOLO (Sensor: Moisture soil)
   float RHsolo=0;                        //Umidade do Solo final medido
   int16 tempo_on=0, tempo_off=0;         //tempos que controlam a bomba de irrigação
   int16 mediaADRHsolo;                   //Media do conversor AD para leitura de Umidade do Solo
   int16 tempo_bombaOff;                  //Tempo que a bomba de irrigação fica desligada
   int   tempo_bombaOn;                   //Tempo que a bomba de irrigação fica ligada
   int   RHsolominimoregistrado=100;      //Registrador de valor maximo lido de Umidade do solo (RHsolo)
   int   RHsolomaximoregistrado=0;        //Registrador de valor minimo lido de Umidade do solo (RHsolo)
   int   RHdesSOLO=0;                     //RH desejado SOLO (Parametrizavel)
   int   Mododabomba=0;                   //Variavel para controlar o modo da bomba (Ligado=1/Automatico=0/Timer=2)
   int   horabombaligada=0;               //Hora programada para a bomba ligar (MODO TIMER)
   int   horabombadesligada=0;            //Hora programada para a bomba desligar (MODO TIMER)
   int   minutobombaligada=0;             //Minuto programado para a bomba ligar (MODO TIMER)
   int   minutobombadesligada=1;          //Minuto programado para a bomba desligar (MODO TIMER)
   signed int   difRHsolo;                //diferença entre o valor parametrizado (RHdesSOLO) eo valor medido (RHsolo)
   int1  Timerdabomba=0;                  //Habilita=1/Desabilita=0 o modo TIMER na bomba
   int1  timersobeautomatico=0;           //Modo timer da bomba de irrigação sobe automatico em caso de ERRO 4.
   
   //VARIAVEIS PARA CONTROLE DE UMIDADE DO AR (Sensor: HIH-4000-003)
   float RHar=0;                          //Umidade do AR final medido
   int16 mediaADRHar;                     //Media do conversor AD para leitura de Umidade do AR
   int   RHdesAR=0;                       //RH desejado AR (Parametrizavel)
   int   RHarminimoregistrado=100;        //Registrador de valor maximo lido de Umidade do ar (RHar)
   int   RHarmaximoregistrado=0;          //Registrador de valor minimo lido de Umidade do ar (RHsar)
   int1  Mododoumidificador=0;            //Variavel para controlar o modo do Umidificador (Ligado=1/Automatico=0)
   int16   tempo_on_2=0;
   int16   tempo_off_2=0;
   int     tempo_bombaOn_2;
   int16   tempo_bombaOff_2;
   signed int difRHar;
   
   //VARIAVEIS AUXILIARES
   float tensao;                          //Auxiliar para valores de tensão dos sensores
   int16 valor;                           //Auxiliar para valores dos Conversores AD
   int   H=0,M=0,S=0;                     //RELOGIO: HORA, MINUTO, SEGUNDO
   int   controletempo=0;                 //Auxiliar para os Segundos do relogio
   int   leituraEeprom;                   //Auxiliar para ler um endereço da memória eeprom e outras funções
   int   rep;                             //Auxiliar para repetiçoes (FOR)
   int   controledestring=0;              //Auxiliar para controle das strings
   int   controledestring2=0;             //Auxiliar para controle das strings 2
   int   controledestring3=0;             //Auxiliar para controle das strings 3
   int   controle=0;                      //Auxiliar contador de tempo para controle de strings 
   int   controle2=0;                     //Auxiliar contador de tempo para controle de strings 2
   int   controle3=0;                     //Auxiliar contador de tempo para controle de strings 3
   int   controle4=0;
   int   controleserial=0;
   int   teclapressionada=0;              //Verifica se a tecla C1 foi pressionada
   int   teclapressionadaMenu=0;          //Verifica se a tecla C0 foi pressionada
   int   teclapressionadaMenu2=0;         //Verifica se a tecla C0 foi pressionada 2
   int   somadosErros=0;                  //Soma os erros em Hexadecimal
   int   MemoriadosErros=0;               //Guarda os valores da soma de erros
   int1  erro1=0;                         //Auxiliar para indicar erros
   int   erro2=0;                         //Auxiliar para indicar erros
   int   erro4=0;                         //Auxiliar para indicar erros
   int   erro8=0;                         //Auxiliar para indicar erros
   int1  auxiliardeErros1;
   int   auxiliardeErros2;
   int   auxiliardeErros4;
   int   auxiliardeErros8;
   int1  verificateclaC1=0;               //Verifica se a tecla C1 permanece apertada
   int1  verificateclaC0=0;               //Verifica se a tecla C0 permanece apertada
   int   timerErro=0;  
//.......................................................................................................   
   //HABILITA LCD E PORTAS ANALOGICAS
   lcd_ini();                             //Inicializa lcd
   delay_ms(300);                         //Aguarda lcd estar pronto
   setup_ADC_ports (AN0_AN1_AN3);         //Habilita portas Analógicas
   setup_ADC(ADC_CLOCK_INTERNAL);         //Conversor AD em 4MHz (CLOCK INTERNO)
   
   // LÊ VALORES ANTES SALVOS NA MEMORIA EEPROM
   Tmin=                 read_eeprom (1);            
   Tmax=                 read_eeprom (2);
   RHdesSOLO=            read_eeprom (3);
   RHdesAR=              read_eeprom (4);
   horabombaligada=      read_eeprom (5);
   horabombadesligada=   read_eeprom (6);         
   minutobombaligada=    read_eeprom (7); 
   minutobombadesligada= read_eeprom (8);
   auxiliardeErros1=     read_eeprom (9);
   auxiliardeErros2=     read_eeprom (10);
   auxiliardeErros4=     read_eeprom (11);
   auxiliardeErros8=     read_eeprom (12);
   
   MemoriadosErros=AuxiliardeErros1+AuxiliardeErros2+AuxiliardeErros4+AuxiliardeErros8;
//.............................................................................................................   
   //INICIALIZAÇÃO DO PROGRAMA E APRESENTAÇÃO DO PROJETO (SOMENTE AO LIGAR) 
   lcd_escreve ('\f');
   lcd_pos_xy(3,1);
   printf (lcd_escreve,"CLICONSOLES       ");  //Climatização e Controle do solo em estufa
   delay_ms(3000);
   lcd_escreve ('\f');
   lcd_pos_xy(5,1);
   printf (lcd_escreve,"Henrique");           //Apresentação
   lcd_pos_xy(2,2);
   printf (lcd_escreve, "    Xavier     ");
   delay_ms(2500);
   
//.................................................................................................................   
   while (TRUE)
   {
   
       //1-CONTROLE DE STRINGS
      //STRINGS PRINCIPAIS
      if (controledestring==0&&controledestring2==0)
      {
        lcd_pos_xy(1,1);
        printf (lcd_escreve, "%1.1f%cC %03.0f%s RHar",Temperatura,0xDF,RHar,"%"); 
        lcd_pos_xy(1,2);
        printf (lcd_escreve, "%03.0f%s RHsolo%c%lu   ", RHsolo,"%",0x7E,tempo_off);
        teclapressionada=0;
      }
      
      if(!input(pin_c1)&&verificateclaC1==0&&teclapressionada<4)
      {
        teclapressionada++;
        controle=0;
      }
      if(!input(pin_c2))
      {
       controle=0;
      }
      if(!input(pin_c1))
      {
        verificateclaC1=1;
      } else verificateclac1=0;
      if(teclapressionada==1&&controledestring2==0)
      {
       controledestring=1;
       lcd_escreve ('\f');
      }
      if(teclapressionada==2&&controledestring2==0)
      {
       controledestring=2;
       lcd_escreve ('\f');
      }
      if(teclapressionada==3&&controledestring2==0)
      {
       controledestring=3;
       lcd_escreve ('\f');
      }
      if(teclapressionada==4&&controledestring2==0)
      {
       controledestring=4;
       lcd_escreve ('\f');
      }
//.......................................................................................................      
      //STRING DOS ERROS
      if(somadosErros!=0&&controledestring2==0&&controle3<=30&&controledestring==0)
      {
       lcd_escreve ('\f');
       lcd_pos_xy(1,1);
       printf(lcd_escreve,"    ALERTA!    ");
       lcd_pos_xy(8,2);
       printf(lcd_escreve,"%02d       ",somadosErros);
      }
      
      //STRING DE REGISTROS
      if(controledestring==1)
      {
        lcd_pos_xy(1,1);
        printf (lcd_escreve, "Temperatura Reg."); 
        lcd_pos_xy(1,2);
        printf (lcd_escreve, "Min:%02u%c  Max:%02u%c",Temperaturaminimaregistrada,0XDF,Temperaturamaximaregistrada,0XDF);
        controle++;
      }
      if(controledestring==2)
      {
        lcd_pos_xy(1,1);
        printf (lcd_escreve, "Umid. Solo  Reg."); 
        lcd_pos_xy(1,2);
        printf (lcd_escreve, "Min:%03u  Max:%03u",RHsolominimoregistrado,RHsolomaximoregistrado);
        controle++;
      }
      if(controledestring==3)
      {
        lcd_pos_xy(1,1);
        printf (lcd_escreve, "Umid. Ar    Reg."); 
        lcd_pos_xy(1,2);
        printf (lcd_escreve, "Min:%03u  Max:%03u",RHarminimoregistrado,RHarmaximoregistrado);
        controle++;
      }
      if(controledestring==4)
      {
        lcd_pos_xy(5,1);
        printf(lcd_escreve,"%02u:%02u:%02u         ",H,M,S);
        lcd_pos_xy(1,2);
        printf(lcd_escreve,"Reg. Alertas: %02d",memoriadosErros);
        controle++;
      }
     
//....................................................................................................................     
     //STRINGS PARA OS ACIONAMENTOS
      if(!input(pin_c0)&&verificateclaC0==0&&controledestring==0&&teclapressionadaMenu<4)
      {
        teclapressionadaMenu++;
        controle=0;
      } 
      if(!input(pin_c0))
      {
        verificateclaC0=1;
      } else verificateclac0=0; 
      if(teclapressionadaMenu==1)
      {
       controledestring2=1;
       lcd_escreve ('\f');
      }
      if(teclapressionadaMenu==2)
      {
       controledestring2=2;
       lcd_escreve ('\f');
      }
      if(teclapressionadaMenu==3)
      {
       controledestring2=3;
       lcd_escreve ('\f');
      }
      if(teclapressionadaMenu==4)
      {
       controledestring2=4;
       lcd_escreve ('\f');
      }
      
      if(controledestring2==1)
      {
        lcd_pos_xy(1,1);
        printf (lcd_escreve, "Aquecimento"); 
        if(Mododoaquecedor==0)
        {
         lcd_pos_xy(1,2);
         printf (lcd_escreve, "Modo: Automatico");
        }
        if(Mododoaquecedor==1)
        {
         lcd_pos_xy(1,2);
         printf (lcd_escreve, "Modo: Ligado    ");
        }
        controle++;
      }
      if(controledestring2==2)
      {
        lcd_pos_xy(1,1);
        printf (lcd_escreve, "Resfriamento"); 
        if(Mododoresfriador==0)
        {
         lcd_pos_xy(1,2);
         printf (lcd_escreve, "Modo: Automatico");
        }
        if(Mododoresfriador==1)
        {
         lcd_pos_xy(1,2);
         printf (lcd_escreve, "Modo: Ligado    ");
        }
        controle++;
      }
      if(controledestring2==3)
      { 
        lcd_pos_xy(1,1);
        printf (lcd_escreve, "Umidificacao");
        if(Mododoumidificador==0)
        {
         lcd_pos_xy(1,2);
         printf (lcd_escreve, "Modo: Automatico");
        }
        if(Mododoumidificador==1)
        {
         lcd_pos_xy(1,2);
         printf (lcd_escreve, "Modo: Ligado    ");
        }
        controle++;
      }
      if(controledestring2==4)
      {
        lcd_pos_xy(1,1);
        printf (lcd_escreve, "Irrigacao"); 
        if(Mododabomba==0)
        {
         lcd_pos_xy(1,2);
         printf (lcd_escreve, "Modo: Automatico");
        }
        if(Mododabomba==1)
        {
         lcd_pos_xy(1,2);
         printf (lcd_escreve, "Modo: Ligado    ");
        }
        if(Mododabomba==2)
        {
         lcd_pos_xy(1,2);
         printf (lcd_escreve, "Modo: Timer     ");
        }
        controle++;
      }
      if(Mododoaquecedor==1)
      {
       output_high(pin_D2);
      } 
      if(Mododoaquecedor==0&&aquec_aut==0)
      {
       output_low(pin_D2);
      } 
      if(Mododoresfriador==1)
      {
       output_high(pin_D3);
      } 
      if(mododoresfriador==0&&resfr_aut==0)
      {
       output_low(pin_D3);
      } 
      if(Mododoumidificador==1&&erro1==0)
      {
       output_high(pin_D5);
      } 
      if(mododoumidificador==0&&tempo_off_2>0)
      {
       output_low(pin_D5);
      }
      if(mododoumidificador==0&&tempo_on_2==0)
      {
       output_low(pin_D5);
      } 
      if(Mododabomba==1&&erro1==0)
      {
       output_high(pin_D4);
      } 
      if(mododabomba==0&&tempo_off>0)
      {
       output_low(pin_D4);
      }
      if(mododabomba==0&&tempo_on==0)
      {
       output_low(pin_D4);
      } 
      if(!input(pin_c1)&&controledestring2==1)
      {
        Mododoaquecedor=1;
      }
      if(!input(pin_c1)&&controledestring2==2)
      {
        Mododoresfriador=1;
      }
      if(!input(pin_c1)&&controledestring2==3&&erro1==0)
      {
        Mododoumidificador=1;
        tempo_on_2=0;
        tempo_off_2=0;
      }
      if(!input(pin_c1)&&controledestring2==4&&timerdabomba==0&&erro1==0)
      {
        Mododabomba=1;
        tempo_on=0;
        tempo_off=0;
      }
      
      if(!input(pin_c2)&&controledestring2==1)
      {
        Mododoaquecedor=0;
      }
      if(!input(pin_c2)&&controledestring2==2)
      {
        Mododoresfriador=0;
      }
      if(!input(pin_c2)&&controledestring2==3)
      {
        Mododoumidificador=0;
      }
      if(!input(pin_c2)&&controledestring2==4&&timerdabomba==0)
      {
        Mododabomba=0;
      }
      
      if(controle>=150||!input(pin_c3))
      {
       controledestring=0;
       controledestring2=0;
       verificateclaC1=0;
       teclapressionada=0;
       verificateclaC0=0;
       teclapressionadaMenu=0;
       controle=0;
      }
//.........................................................................................................     
     
       //2-LEITURA DOS SENSORES      
      //LEITURA DE TEMPERATURA
      set_ADC_channel(0);
      delay_us(20);
      mediaADtpt=0; valor=0; tensao=0;
      for(rep=1; rep<=8; rep++)
      {
        valor= read_adc();
        delay_ms(4);
        mediaADtpt= mediaADtpt + valor;
      }
      mediaADtpt= mediaADtpt/8;
      tensao=(float)mediaADtpt*4.883;
      Temperatura=(float) tensao/57.7394;
      
  
      
      //LEITURA DE UMIDADE DO SOLO(RHsolo)
      set_ADC_channel(1);
      delay_us(20);
      mediaADRHsolo=0; valor=0; tensao=0;
      for(rep=1; rep<=8; rep++)
      {
        valor= read_adc();
        delay_us(4100);
        mediaADRHsolo= mediaADRHsolo + valor;
      }
      mediaADRHsolo= mediaADRHsolo/8;
      tensao=(float) mediaADRHsolo*4.883;
      RHsolo=(float)(tensao-1100)/39;
      RHsolo=(float)100 - RHsolo;
      
      //LEITURA DE UMIDADE DO AR(RHar)
      set_ADC_channel(3);
      delay_us(20);
      mediaADRHar=0; valor=0; tensao=0;
      for(rep=1; rep<=9; rep++)
      {
        valor= read_adc();
        delay_ms(4);
        mediaADRHar= mediaADRHar + valor;
      }
      mediaADRHar= mediaADRHar/9;
      tensao=(float) mediaADRHar*4.883;
      RHar=(float)(tensao-840)/31;
//.....................................................................................................     
      
       //3-REGISTRO DE VALORES MINIMOS E MAXIMOS   
      //Temperatura
      if(Temperatura<Temperaturaminimaregistrada||!input(pin_c2)&&controledestring==1)
      {
        Temperaturaminimaregistrada=Temperatura;
      }
      if(Temperatura>Temperaturamaximaregistrada||!input(pin_c2)&&controledestring==1)
      {
        Temperaturamaximaregistrada=Temperatura;
      }
      //Umidade do solo
      if(RHsolo<RHsolominimoregistrado||!input(pin_c2)&&controledestring==2)
      {
        RHsolominimoregistrado=RHsolo;
      } 
      if(RHsolo>RHsolomaximoregistrado||!input(pin_c2)&&controledestring==2)
      {
        RHsolomaximoregistrado=RHsolo;
      }
      //Umidade do AR
      if(RHar<RHarminimoregistrado||!input(pin_c2)&&controledestring==3)
      {
        RHarminimoregistrado=RHar;
      }
      if(RHar>RHarmaximoregistrado||!input(pin_c2)&&controledestring==3)
      {
        RHarmaximoregistrado=RHar;
      }
      if(!input(pin_c2)&&controledestring==4)
      {
        memoriadosErros=0;
        auxiliardeErros1=0;
        auxiliardeErros2=0;
        auxiliardeErros4=0;
        auxiliardeErros8=0;
      }
//......................................................................................................      
      
       //4-SEGURANÇA E PROTEÇÕES 
      //ERRO DE FALTA DE AGUA NO RESERVATORIO
      if(input(pin_c4))     // Sensor de nivel no resevatório de agua (C4)
      { 
       timerErro++;
       if(timerErro==20)
       {
        erro1=1;
        tempo_on=0;
        tempo_off=0;
        tempo_on_2=0;
        tempo_off_2=0;
        output_low(pin_d4);
        output_low(pin_d5);
       }
      }
      else {erro1=0;timerErro=0;}
      
      //ERRO NO SENSOR DE TEMPERATURA
      if(mediaADtpt<5)
      {
       erro2=2;
       output_low(pin_d2);
       output_low(pin_d3);
      }
      else {erro2=0;}
      
      //ERRO NO SENSOR DE UMIDADE DO SOLO
      if(mediaADRHsolo<100||mediaADRHsolo>1000)
      {
       erro4=4;
       timersobeautomatico=1;
       tempo_on=0;
       tempo_off=0;
       if(mododabomba==0)
       {output_low(pin_d4);}
       RHsolo=0;
      }
      else {erro4=0; timersobeautomatico=0;}
      
      //ERRO NO SENSOR DE UMIDADE DO AR
      if(mediaADRHar<2)
      {
       erro8=8;
       output_low(pin_d5);
       RHar=0;
       tempo_on_2=0;
       tempo_off_2=0;
       if(mododoumidificador==0)
       {output_low(pin_d5);}
      }
      else {erro8=0;}
      
      somadosErros=erro1+erro2+erro4+erro8;
      
      if(somadosErros!=0)
      {
       controle4++;
       if(controle4>10)
       {
        if(AuxiliardeErros1==0)
        AuxiliardeErros1=erro1;
       
        if(AuxiliardeErros2==0)
        AuxiliardeErros2=erro2;
       
        if(AuxiliardeErros4==0)
        AuxiliardeErros4=erro4;
       
        if(AuxiliardeErros8==0)
        AuxiliardeErros8=erro8;
      
        MemoriadosErros=AuxiliardeErros1+AuxiliardeErros2+AuxiliardeErros4+AuxiliardeErros8;
        controle4=0;
       } 
      }
      
      if(somadoserros==0)
      {
       output_low(pin_D7);
       controle2=0;
       controle3=0;
      }
      
      if(somadoserros!=0)
      {
       if(controle2<=5)
       {output_high(pin_d7);}
       controle2++;
       if(controle2>5)
       {output_low(pin_d7);}
       if(controle2==10)
       {controle2=0;}
       controle3++;
      }
      if(controle3>50)
      {controle3=0;}

//...................................................................................................

       //5-CONTROLE DE SAIDAS
      //AQUECIMENTO(D2) E RESFRIAMENTO(D3)
      if (Temperatura<Tmin&&mododoaquecedor==0&&erro2==0)              
      {
       aquec_aut=1;
       output_high(pin_D2);
      } 
      if (Temperatura>Tmin&&mododoaquecedor==0)
      {
       output_low(pin_D2);
      }
      if (Temperatura>Tmax&&mododoresfriador==0&&erro2==0) 
      {
       resfr_aut=1;
       output_high(pin_D3);
      }
      if (Temperatura<Tmax&&mododoresfriador==0) 
      {
       output_low(pin_D3);
      }
      
      //UMIDIFICAÇÃO (D5) 
      if(RHar>=RHdesAR&&Mododoumidificador==0) 
      {
       output_low(pin_D5);
       tempo_on_2=0;
       tempo_off_2=0;
       tempo_bombaOn_2=0;
       tempo_bombaOff_2=0;
      }
      difRHar= RHdesAR - RHar;
      if (RHar<RHdesAr&&difRHar>0.00&&Mododoumidificador==0&&erro1==0&&erro8==0)          
      {
       
         tempo_bombaOff_2 = 40;
         tempo_bombaOn_2 = 40;
        
        tempo_on_2++;                                    
        if (tempo_off_2==0&&difRHar>0)                
        {
         output_high(pin_D5);
        }
        if(tempo_on_2==tempo_bombaOn_2)
        {
         tempo_off_2 = tempo_on_2; 
         output_low(pin_D5);     
        } 
        if(tempo_off_2>=tempo_bombaOn_2&&difRHar>0)       
        {
         tempo_off_2++;           
        }
        if (tempo_off_2>=tempo_bombaOff_2)                  
        {
         tempo_on_2=0;
         tempo_off_2=0;
        }
      }
      
      //IRRIGAÇÃO (D4)
      if(RHsolo>=RHdesSolo&&Mododabomba==0) 
      {
       output_low(pin_D4);
       tempo_on=0;
       tempo_off=0;
       tempo_bombaOn=0;
       tempo_bombaOff=0;
      }
      difRHsolo= RHdesSOLO - RHsolo;
      if (RHsolo<RHdesSOLO&&difRHsolo>0.00&&Mododabomba==0&&erro1==0&&erro4==0&&timersobeautomatico==0)          
      {
        if (difRHsolo>75)
        {
         tempo_bombaOff = 200;
         tempo_bombaOn=50;
        }
        if (difRHsolo>50&& difRHsolo<=75)
        {
         tempo_bombaoff = 300;
         tempo_bombaOn= 40;
        }
        if (difRHsolo>20&& difRHsolo<=50)
        {
         tempo_bombaoff = 400;
         tempo_bombaOn= 30;
        }
        if (difRHsolo<=20&&difRHsolo>0)
        {
         tempo_bombaoff = 500;
         tempo_bombaOn= 20;
        }
        
        tempo_on++;                                    
        if (tempo_off==0&&difRHsolo>0)                
        {
         output_high(pin_D4);
        }
        if(tempo_on==tempo_bombaOn)
        {
         tempo_off = tempo_on; 
         output_low(pin_D4);     
        } 
        if(tempo_off>=tempo_bombaOn&&difRHsolo>0)       
        {
         tempo_off++;           
        }
        if (tempo_off>=tempo_bombaOff)                  
        {
         tempo_on=0;
         tempo_off=0;
        }
      }
   
      
//6-MENU DE PARAMETRIZAÇÃOES      
      //PARAMETRIZA VALOR DE TEMPERATURA MINIMA (Tmin)
       if(!input(pin_c0))                       
       {
        teclapressionadaMenu2++;
       }
       if(input(pin_c0))
       {
        teclapressionadaMenu2=0;
       }
       if(teclapressionadaMenu2==15)                                
       {
        controledestring=0;
        controledestring2=0;
        controledestring3=0;
        teclapressionada=0;
        verificateclaC1=0;
        teclapressionadaMenu=0;
        verificateclaC0=0;
        teclapressionadaMenu2=0;
        lcd_escreve ('\f');
        
        while (input(pin_c3))
        {
         delay_ms(100);
         output_d(0);           
         if(!input(pin_c0)&&verificateclaC1==0)         
         {
          teclapressionadaMenu2++;
         }
         if(timerdabomba==0)
         {
          if(teclapressionadaMenu2>7)
          {teclapressionadaMenu2=7;}
         }
         if(!input(pin_c0))
         {
          verificateclaC1=1;
         }else verificateclaC1=0;
         if(teclapressionadaMenu2==1)
         {
          controledestring3=1;
          lcd_escreve ('\f');
         }
         if(teclapressionadaMenu2==2)
         {
          controledestring3=2;
          lcd_escreve ('\f');
         }
         if(teclapressionadaMenu2==3)
         {
          controledestring3=3;
          lcd_escreve ('\f');
         }
         if(teclapressionadaMenu2==4)
         {
          controledestring3=4;
          lcd_escreve ('\f');
         }
         if(teclapressionadaMenu2==5)
         {
          controledestring3=5;
          lcd_escreve ('\f');
         }
         if(teclapressionadaMenu2==6)
         {
          controledestring3=6;
          lcd_escreve ('\f');
         }
         if(teclapressionadaMenu2==7)
         {
          controledestring3=7;
          lcd_escreve ('\f');
         }
         if(teclapressionadaMenu2==8)
         {
          controledestring3=8;
          lcd_escreve ('\f');
         }
         if(teclapressionadaMenu2==9)
         {
          controledestring3=9;
          lcd_escreve ('\f');
         }
         if(teclapressionadaMenu2==10)
         {
          controledestring3=10;
          lcd_escreve ('\f');
         }
         if(teclapressionadaMenu2==11)
         {
          controledestring3=11;
          lcd_escreve ('\f');
         }
         if(controledestring3==1)
         {
          lcd_pos_xy(1,1);
          printf (lcd_escreve, "Temperatura");
          lcd_pos_xy(1,2);
          printf (lcd_escreve, "Offset:     %02d%cC",Tmin,0xDF);
          if(!input(pin_c1))                            //TECLA SETA PRA CIMA
          {Tmin++;}
          if(!input(pin_c2))                           //TECLA SETA PARA BAIXO
          {Tmin--;}
          if(Tmin<1)                                   // Estipula limites para Tmin
          {Tmin=1;} 
          if(Tmin>60)
          {Tmin=60;}
         }
         if(controledestring3==2)
         {
          lcd_pos_xy(1,1);
          printf (lcd_escreve, "Temperatura");
          lcd_pos_xy(1,2);
          printf (lcd_escreve, "Maxima:     %02d%cC",Tmax,0xDF);
          if(!input(pin_c1))                  //TECLA Seta pata cima
          {Tmax++;}
          if(!input(pin_c2))                  //TECLA Seta para baixo
          {Tmax--;}
          if(Tmax<20)                         //Estipula limites para Tmax
          {Tmax=20;}
          if(Tmax>61)
          {Tmax=61;}
         }
         if(controledestring3==3)
         {
          lcd_pos_xy(1,1);
          printf (lcd_escreve, "Umidade do Solo ");
          lcd_pos_xy(1,2);
          printf (lcd_escreve, "Offset:     %03d%s",RHdesSOLO,"%");
          if(!input(pin_c1))                     //TECLA SETA PARA CIMA
          {RHdesSOLO++;}
          if(!input(pin_c2))                     //TECLA SETA PARA BAIXO
          {RHdesSOLO--;}
          if(RHdesSOLO<1)                        //Estipula Limites para RHdesSOLO
          {RHdesSOLO=1;}
          if(RhdesSOLO>100)  
          {RHdesSOLO=100;}
         }
         if(controledestring3==4)
         { 
          lcd_pos_xy(1,1);
          printf (lcd_escreve, "Umidade do AR ");
          lcd_pos_xy(1,2);
          printf (lcd_escreve, "Offset:     %03d%s",RHdesAR,"%");
          if(!input(pin_c1))                   //TECLA SETA PARA CIMA
          {RHdesAR++;}
          if(!input(pin_c2))                   //TECLA SETA PARA BAIXO
          {RHdesAR--;}
          if(RHdesAR<1)                        //Estipula Limites para RHdes
          {RHdesAR=1;}
          if(RhdesAR>100)  
          {RHdesAR=100;}
         }       
         if(controledestring3==5)
         {
          leituraeeprom=H;
          lcd_pos_xy(1,1);
          printf(lcd_escreve,"Ajustar Relogio");
          lcd_pos_xy(5,2);
          printf(lcd_escreve,"%c%02d:%02d      ",0x7E,H,M);
          if(!input(pin_c1))
          {H++;}
          if(!input(pin_c2))
          {H--;}
          if(H<1)         //Estipula limites para H
          {H=0;}
          if(H>23)
          {H=23;}
          if(leituraeeprom!=H)
          {S=0;}
         }
         if(controledestring3==6)
         {
          leituraeeprom=M;
          lcd_pos_xy(1,1);
          printf(lcd_escreve,"Ajustar Relogio");
          lcd_pos_xy(6,2);
          printf(lcd_escreve,"%02d:%02d%c      ",H,M,0x7F);
          if(!input(pin_c1))
          {M++;}
          if(!input(pin_c2))
          {M--;}
          if(M<1)         //Estipula limites para M
          {M=0;}
          if(M>59)
          {M=59;}
          if(leituraeeprom!=M)
          {S=0;}
         }
         if(controledestring3==7)
         {
          lcd_pos_xy(1,1);
          printf(lcd_escreve,"Timer Irrigacao ");
          if(timerdabomba==0&&timersobeautomatico==0)
          {
           lcd_pos_xy(1,2);
           printf(lcd_escreve,"Desabilitado    ");
           mododabomba=0;
          }
          if(timerdabomba==1||timersobeautomatico==1)
          {
           lcd_pos_xy(1,2);
           printf(lcd_escreve,"Habilitado      ");
           mododabomba=2;
           tempo_on=0;
           tempo_off=0;
          }
          if(!input(pin_c1))
          {timerdabomba=1;}
          if(!input(pin_c2))
          {timerdabomba=0;}
        }
        if(controledestring3==8)
        {
         lcd_pos_xy(1,1);
         printf(lcd_escreve,"Timer Irrigacao");
         lcd_pos_xy(1,2);
         printf(lcd_escreve,"Ligar:   %c%02d:%02d ",0x7E,horabombaligada,minutobombaligada);
         if(!input(pin_c1))
         {horabombaligada++;}
         if(!input(pin_c2))
         {horabombaligada--;}
         if(horabombaligada<1)
         {horabombaligada=0;}
         if(horabombaligada>23)
         {horabombaligada=23;}
        }
        if(controledestring3==9)
        {
         lcd_pos_xy(1,1);
         printf(lcd_escreve,"Timer Irrigacao");
         lcd_pos_xy(1,2);
         printf(lcd_escreve,"Ligar:    %02d:%02d%c",horabombaligada,minutobombaligada,0x7F);
         if(!input(pin_c1))
         {minutobombaligada++;}
         if(!input(pin_c2))
         {minutobombaligada--;}
         if(minutobombaligada<1)
         {minutobombaligada=0;}
         if(minutobombaligada>59)
         {minutobombaligada=59;}
        }
        if(controledestring3==10)
        {
         lcd_pos_xy(1,1);
         printf(lcd_escreve,"Timer Irrigacao");
         lcd_pos_xy(1,2);
         printf(lcd_escreve,"Desligar:%c%02d:%02d ",0x7E,horabombadesligada,minutobombadesligada);
         if(!input(pin_c1))
         {horabombadesligada++;}
         if(!input(pin_c2))
         {horabombadesligada--;}
         if(horabombadesligada<1)
         {horabombadesligada=0;}
         if(horabombadesligada>23)
         {horabombadesligada=23;}
        }
        if(controledestring3==11)
        {
         lcd_pos_xy(1,1);
         printf(lcd_escreve,"Timer Irrigacao");
         lcd_pos_xy(1,2);
         printf(lcd_escreve,"Desligar: %02d:%02d%c",horabombadesligada,minutobombadesligada,0x7F);
         if(!input(pin_c1))
         {minutobombadesligada++;}
         if(!input(pin_c2))
         {minutobombadesligada--;}
         if(minutobombadesligada<1)
         {minutobombadesligada=0;}
         if(minutobombadesligada>59)
         {minutobombadesligada=59;}
        }
        
        controletempo++;
        if(controletempo>=10)
        {S++;controletempo=0;}
        if(S>59)
        {S=0;M++;}
        if(M>59)
        {S=0;M=0;H++;}
        if(H>23)
        {H=0; M=0; S=0;}
      
      }//while menu
   }// if menu
   if(!input(pin_c3))        
   {
    controledestring=0;
    controledestring2=0;
    controledestring3=0;
    verificateclaC1=0;
    teclapressionada=0;
    verificateclaC0=0;
    teclapressionadaMenu=0;
    teclapressionadaMenu2=0;
    controle=0;
   }

//RELOGIO
   controletempo++;
   if(controletempo>=8)
   {S++;controletempo=0;}
   if(S>59)
   {S=0;M++;}
   if(M>59)
   {S=0;M=0;H++;}
   if(H>23)
   {H=0; M=0; S=0;}
                  
//TIMER DA BOMBA
 /*  if(timerdabomba==0&&timersobeautomatico==0)
   {
     mododabomba=0;
   } */
   
   if(timersobeautomatico==1)
   {
    mododabomba=2;
    tempo_on=0;
    tempo_off=0;
   }
   
   
   if(horabombaligada==horabombadesligada&&minutobombaligada==minutobombadesligada)
   {minutobombadesligada++;}
   
   if(timerdabomba==1&&minutobombaligada==M&&horabombaligada==H&&erro1==0)
   {
    output_high(pin_D4);
   }
   if(timersobeautomatico==1&&minutobombaligada==M&&horabombaligada==H&&erro1==0)
   {
    output_high(pin_D4);
   } 
   
   if(timerdabomba==1&&minutobombadesligada==M&&horabombadesligada==H)
   {
    output_low(pin_D4);
   }
   if(timersobeautomatico==1&&minutobombadesligada==M&&horabombadesligada==H)
   {
    output_low(pin_D4);
   }
   
//Salva valores
   leituraEeprom=read_eeprom(1);
   if(leituraEeprom!=Tmin)
   {write_eeprom(1,Tmin);}
   
   leituraEeprom=read_eeprom(2);
   if(leituraEeprom!=Tmax)
   {write_eeprom(2,Tmax);}
   
   leituraEeprom=read_eeprom(3);
   if(leituraEeprom!=RHdesSOLO)
   {write_eeprom(3,RHdesSOLO);tempo_on=0;tempo_off=0;}
   
   leituraEeprom=read_eeprom(4);
   if(leituraEeprom!=RHdesAR)
   {write_eeprom(4,RHdesAR);}
   
   leituraEeprom=read_eeprom(5);
   if(leituraEeprom!=horabombaligada)
   {write_eeprom(5,horabombaligada);}
   
   leituraEeprom=read_eeprom(6);
   if(leituraEeprom!=horabombadesligada)
   {write_eeprom(6,horabombadesligada);}
   
   leituraEeprom=read_eeprom(7);
   if(leituraEeprom!=minutobombaligada)
   {write_eeprom(7,minutobombaligada);}
   
   leituraEeprom=read_eeprom(8);
   if(leituraEeprom!=minutobombadesligada)
   {write_eeprom(8,minutobombadesligada);}
   
   leituraEeprom=read_eeprom(9);
   if(leituraEeprom!=auxiliardeErros1)
   {write_eeprom(9,auxiliardeErros1);}
   
   leituraEeprom=read_eeprom(10);
   if(leituraEeprom!=auxiliardeErros2)
   {write_eeprom(10,auxiliardeErros2);}
   
   leituraEeprom=read_eeprom(11);
   if(leituraEeprom!=auxiliardeErros4)
   {write_eeprom(11,auxiliardeErros4);}
   
   leituraEeprom=read_eeprom(12);
   if(leituraEeprom!=auxiliardeErros8)
   {write_eeprom(12,auxiliardeErros8);}
   
   
//COMANDOS SERIAL
   
   if(comando==1)
    {printf("Temperatura= %2.1f graus \r\n", Temperatura);
     printf("Umidade do Ar= %2.1f %s \r\n", RHar,"%");
     printf("Umidade do Solo= %2.1f %s \r\n", RHsolo,"%");
     comando=0;}
     
   if(comando==2)
   {mododoaquecedor=1;
    printf("Aquecedor foi ligado manualmente \r\n");
    comando=0;}
    
   if(comando==3)
   {mododoaquecedor=0;
    printf("Aquecedor foi para modo automatico \r\n");
    comando=0;}
    
    if(comando==4)
   {mododoresfriador=1;
    printf("Resfriador foi ligado manualmente \r\n");
    comando=0;}
    
   if(comando==5)
   {mododoresfriador=0;
    printf("Resfriador foi para modo automatico \r\n");
    comando=0;}
    
    if(comando==6)
   {mododoumidificador=1;
    printf("Umidificador foi ligado manualmente \r\n");
    comando=0;}
    
   if(comando==7)
   {mododoumidificador=0;
    printf("Umidificador foi para modo automatico \r\n");
    comando=0;}
    
    if(comando==8)
   {mododabomba=1;
    printf("Irrigador foi ligado manualmente \r\n");
    tempo_on=0;
    tempo_off=0;
    comando=0;}
    
   if(comando==9)
   {mododabomba=0;
    printf("Irrigador foi para modo automatico \r\n");
    comando=0;}
    
    if(comando==10)
    {printf("Temperatura minima registrada= %u graus \r\n",      Temperaturaminimaregistrada);
     printf("Temperatura maxima registrada = %u graus \r\n",     Temperaturamaximaregistrada);
     printf("Umidade do Ar maxima registrada= %u %s \r\n",    RHarminimoregistrado,"%");
     printf("Umidade do Ar minima registrada= %u %s  \r\n",   RHarmaximoregistrado,"%");
     printf("Umidade so Solo minima registrada = %u %s \r\n", RHsolominimoregistrado,"%");
     printf("Umidade do Solo maxima registrada= %u %s \r\n",  RHsolomaximoregistrado,"%");
     comando=0;}
    
    if(comando==11)
    {printf("Registro de Alertas= %d \r\n", memoriadosErros);
     comando=0;}
     
    if(somadosErros!=0)
    {controleserial++;
     if(controleserial>100)
     {printf("ALERTA! Algo está errado. Codigo: %d\r\n", somadosErros);
      controleserial=0;}
    } 
    
  }//true
}//main

