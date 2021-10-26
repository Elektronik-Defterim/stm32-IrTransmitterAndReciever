#include <stm32f1xx_hal.h>
#include "irRemote.h"
#include "flashPage.h"

uint32_t irData,transmitIrData;
uint32_t IrDatas[5] = {1,2,3,4,5};
uint8_t state = 0;

extern TIM_HandleTypeDef htim1;

void delayUs(uint16_t s){
	//TIM1 zamanlay�c� olarak 72Mhz frekans�nda �al���r.Prescaler degerini 72-1 al�rsak 72/72 = 1us'e tekabul eder
	__HAL_TIM_SET_COUNTER(&htim1,0);
	while(__HAL_TIM_GET_COUNTER(&htim1)<(s));
}

static void waitButton(GPIO_TypeDef *Port,uint16_t pin){
	while((HAL_GPIO_ReadPin(Port,pin)));
}

void controlButton(){
	//Right Button
	 if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_4)){        //A4 Pini set durumunda mi.
		  waitButton(GPIOA,GPIO_PIN_4);             //A4 pininin reset olmas�n� bekle.
		  	  state = 1;
	  }

	  //Button Ok
	  else if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_5)){ //A5 Pini set durumunda mi.
		  waitButton(GPIOA,GPIO_PIN_5);			   //A5 pininin reset olmas�n� bekle.
	 	  	  state = 2;
	 	  }

	  //Left Button
	 else if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_6)){ //A6 Pini set durumunda m�.
		 waitButton(GPIOA,GPIO_PIN_6);			  //A6 pininin reset olmas�n� bekle.
	 	  	  state = 3;
	 	 }

	  //Button Up
	 else if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_7)){ //A7 Pini set durumunda m�.
		 waitButton(GPIOA,GPIO_PIN_7);			  //A7 pininin reset olmas�n� bekle.
	 	  	  state = 4;
	 	 }
	  //Down Button
	 else if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_1)){ //A1 Pini set durumunda m�.
	 		 waitButton(GPIOA,GPIO_PIN_1);		  //A1 pininin reset olmas�n� bekle.
	 	 	  	  state = 5;
	 	 	 }

	 //Recieve moda ge�i� butonu
	 else if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)){ //A0 Pini set durumunda m�.
		 		 waitButton(GPIOA,GPIO_PIN_0);	  //A0 Pininin reset olmas�n� bekle.
		 	 	  	  state = 6;
		}

}

void getIrData(){
	/*Haberle�menin ba�lamas�n� bekler bu s�re�te al�c� ��k��� 1 durumundad�r*/
	while(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0));

	/*Verici 32 bitlik veriyi g�ndermeden 9ms'likLeading Pals g�nderir ve bu s�re�te al�c� ��k��� 0 durumundad�r*/
	while(!(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0)));  //B0 pinini set oluncaya kadar bekle

	/*Verici leading pals'i g�derdikten sonra 4,5ms boyunca bekler bu s�re�te al�c� ��k��� 1 durumundad�r*/
	while((HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0)));  //B0 pinini reset oluncaya kadar bekle
	//leading pals ve yakla��k 4,5ms bekledikten sonra
	uint8_t count;
	  for (int i=0; i<32; i++)
		  {
			  count=0;
			  while (!(HAL_GPIO_ReadPin (GPIOB, GPIO_PIN_0))); //B0 pinini set oluncaya kadar bekle
			  while ((HAL_GPIO_ReadPin (GPIOB, GPIO_PIN_0)))  // B0 pinini set oldugu s�rece d�ng�de kal.
			  {
				  count++;
				  delayUs(100);
			  }

			  if (count > 6) 					//count > 12 1.2ms ise gelen bit 1
			  {
				  irData |= (1UL << (31-i));   //(31-i).bit'e 1 yaz
			  }

			  else irData &= ~(1UL << (31-i));  //(31-i).bit'e 0 yaz
		  }
}

/*GIPGIP: fonksiyonu veri al�nd�ktan veri al�nd�g�n� anlayabilmek i�in �agriliyor.
 ve Al�nan verinin butona kaydedildi�ini kullan�c�n�n anlayabilmesi i�in �a�r�l�yor*/
void GIPGIP(){
	for(int i = 0;i<4;i++){
		HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_11);
		for(int j=0 ; j<10;j++)
			delayUs(25000);
	}
}

void recieveMode(){
	getIrData();								//Gelen K�z�l�tesi veriyi oku
	state = 0;
	GIPGIP();									//Veri al�nd�g�n� kullan�c�n�n anlayabilmesi i�in  B11 pinine ba�l� led 2 kez yak�l�p s�nd�r�l�yor.

	while(state==0){							//Al�nana k�z�l�tesi veriyi kaydetmek i�in,kaydedilecek butona bas�lmas� bekleniyor
		controlButton();
	}

	if(state !=6 && state !=0){
		IrDatas[state-1] = irData;             //Al�nan k�z�l�tesi ver bas�lan butona kaydediliyor
		GIPGIP();							   //Kay�t i�leminin ger�ekle�ti�ini kullan�c�ya anlayabilmesi i�in B11 pinine ba�l� led 2 kez yak�l�p s�nd�r�l�yor.
		flashWrite(0x08004410,IrDatas,5);	   //Veriler flash' a kaydediliyor
		flashRead(0x08004410,IrDatas,5);	   //Veriler flash'tan tekrar okunuyor.
	}
	state = 0;
}

void pulse(){
	//f = 38khz T=26 us T/2 = 13us    13*44=572us
	for(int i = 0; i<44;i++){
			HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_1);
			delayUs(13);
		}
}

void startNec(){
	 //16 defa pulseBurst fonksiyonu cagriliyor 16*572 = 9ms
	for(int i= 0;i<16;i++)
		pulse();
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET);
	//4.5 ms boyunca veri g�ndermeyi b�rak�yoruz.Bu durumda al�c� lojik 1 seviyesindedir.
	delayUs(4500);
}

void tansmitZeroBit(){
	//Ir transmitter 0  g�nderiyor. 1.125ms (44*13+560us)
	pulse();
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET);
	delayUs(560);
}

void transmitOneBit(){
	//Ir transmitter 1 g�nderiyor.Yakla��k 2.25ms = 13*44+1688us
	pulse();
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET);
	delayUs(1687);
}

void transmitData(uint32_t datas){
	for(int j = 0;j<32;j++){
		if((datas>>(31-j))& 0x1)
			transmitOneBit();
		else
			tansmitZeroBit();
	}
}

void endTranmitData(){
	pulse();
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET);
}

/*Verici Mod*/
void irTransmit(int state){
	/*NEC PROTOCOL
    a 9ms leading pulse burst (16 times the pulse burst length used for a logical data bit)
	a 4.5ms space
	the 8-bit address for the receiving device
	the 8-bit logical inverse of the address
	the 8-bit command
	the 8-bit logical inverse of the command
	a final 562.5�s pulse burst to signify the end of message transmission
	transmitIrData = IrDatas[state-1];
	*/
	transmitIrData = IrDatas[state-1];
	/*Nec Protocol*/
	startNec();  //9 ms boyunca 38 khz frekans�nda leading pulse g�nderiyoruz
	transmitData(transmitIrData);
	endTranmitData();
}

void mainProccessCenter(){
	  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_10,GPIO_PIN_SET); //Mavi led
	  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_11,GPIO_PIN_RESET);//Sar� Led
	  flashRead(0x08004410,IrDatas,5);                    //Flash veriler okunuyor

	  while(1){
		  controlButton();
		 if(state == 6){
			 HAL_GPIO_WritePin(GPIOB,GPIO_PIN_10,GPIO_PIN_RESET);
		  	 HAL_GPIO_WritePin(GPIOB,GPIO_PIN_11,GPIO_PIN_SET);
    		 recieveMode();
		  	 HAL_GPIO_WritePin(GPIOB,GPIO_PIN_10,GPIO_PIN_SET);
		  	 HAL_GPIO_WritePin(GPIOB,GPIO_PIN_11,GPIO_PIN_RESET);
		  	 state = 0;
		   }

		  else if(state !=0){
			  irTransmit(state);
		  	  state = 0;
		  }
		  HAL_Delay(100);
	  }
}



