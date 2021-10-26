#include <stm32f1xx_hal.h>
#include "irRemote.h"
#include "flashPage.h"

uint32_t irData,transmitIrData;
uint32_t IrDatas[5] = {1,2,3,4,5};
uint8_t state = 0;

extern TIM_HandleTypeDef htim1;

void delayUs(uint16_t s){
	//TIM1 zamanlayýcý olarak 72Mhz frekansýnda çalýþýr.Prescaler degerini 72-1 alýrsak 72/72 = 1us'e tekabul eder
	__HAL_TIM_SET_COUNTER(&htim1,0);
	while(__HAL_TIM_GET_COUNTER(&htim1)<(s));
}

static void waitButton(GPIO_TypeDef *Port,uint16_t pin){
	while((HAL_GPIO_ReadPin(Port,pin)));
}

void controlButton(){
	//Right Button
	 if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_4)){        //A4 Pini set durumunda mi.
		  waitButton(GPIOA,GPIO_PIN_4);             //A4 pininin reset olmasýný bekle.
		  	  state = 1;
	  }

	  //Button Ok
	  else if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_5)){ //A5 Pini set durumunda mi.
		  waitButton(GPIOA,GPIO_PIN_5);			   //A5 pininin reset olmasýný bekle.
	 	  	  state = 2;
	 	  }

	  //Left Button
	 else if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_6)){ //A6 Pini set durumunda mý.
		 waitButton(GPIOA,GPIO_PIN_6);			  //A6 pininin reset olmasýný bekle.
	 	  	  state = 3;
	 	 }

	  //Button Up
	 else if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_7)){ //A7 Pini set durumunda mý.
		 waitButton(GPIOA,GPIO_PIN_7);			  //A7 pininin reset olmasýný bekle.
	 	  	  state = 4;
	 	 }
	  //Down Button
	 else if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_1)){ //A1 Pini set durumunda mý.
	 		 waitButton(GPIOA,GPIO_PIN_1);		  //A1 pininin reset olmasýný bekle.
	 	 	  	  state = 5;
	 	 	 }

	 //Recieve moda geçiþ butonu
	 else if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)){ //A0 Pini set durumunda mý.
		 		 waitButton(GPIOA,GPIO_PIN_0);	  //A0 Pininin reset olmasýný bekle.
		 	 	  	  state = 6;
		}

}

void getIrData(){
	/*Haberleþmenin baþlamasýný bekler bu süreçte alýcý çýkýþý 1 durumundadýr*/
	while(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0));

	/*Verici 32 bitlik veriyi göndermeden 9ms'likLeading Pals gönderir ve bu süreçte alýcý çýkýþý 0 durumundadýr*/
	while(!(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0)));  //B0 pinini set oluncaya kadar bekle

	/*Verici leading pals'i göderdikten sonra 4,5ms boyunca bekler bu süreçte alýcý çýkýþý 1 durumundadýr*/
	while((HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0)));  //B0 pinini reset oluncaya kadar bekle
	//leading pals ve yaklaþýk 4,5ms bekledikten sonra
	uint8_t count;
	  for (int i=0; i<32; i++)
		  {
			  count=0;
			  while (!(HAL_GPIO_ReadPin (GPIOB, GPIO_PIN_0))); //B0 pinini set oluncaya kadar bekle
			  while ((HAL_GPIO_ReadPin (GPIOB, GPIO_PIN_0)))  // B0 pinini set oldugu sürece döngüde kal.
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

/*GIPGIP: fonksiyonu veri alýndýktan veri alýndýgýný anlayabilmek için çagriliyor.
 ve Alýnan verinin butona kaydedildiðini kullanýcýnýn anlayabilmesi için çaðrýlýyor*/
void GIPGIP(){
	for(int i = 0;i<4;i++){
		HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_11);
		for(int j=0 ; j<10;j++)
			delayUs(25000);
	}
}

void recieveMode(){
	getIrData();								//Gelen Kýzýlötesi veriyi oku
	state = 0;
	GIPGIP();									//Veri alýndýgýný kullanýcýnýn anlayabilmesi için  B11 pinine baðlý led 2 kez yakýlýp söndürülüyor.

	while(state==0){							//Alýnana kýzýlötesi veriyi kaydetmek için,kaydedilecek butona basýlmasý bekleniyor
		controlButton();
	}

	if(state !=6 && state !=0){
		IrDatas[state-1] = irData;             //Alýnan kýzýlötesi ver basýlan butona kaydediliyor
		GIPGIP();							   //Kayýt iþleminin gerçekleþtiðini kullanýcýya anlayabilmesi için B11 pinine baðlý led 2 kez yakýlýp söndürülüyor.
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
	//4.5 ms boyunca veri göndermeyi býrakýyoruz.Bu durumda alýcý lojik 1 seviyesindedir.
	delayUs(4500);
}

void tansmitZeroBit(){
	//Ir transmitter 0  gönderiyor. 1.125ms (44*13+560us)
	pulse();
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET);
	delayUs(560);
}

void transmitOneBit(){
	//Ir transmitter 1 gönderiyor.Yaklaþýk 2.25ms = 13*44+1688us
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
	a final 562.5µs pulse burst to signify the end of message transmission
	transmitIrData = IrDatas[state-1];
	*/
	transmitIrData = IrDatas[state-1];
	/*Nec Protocol*/
	startNec();  //9 ms boyunca 38 khz frekansýnda leading pulse gönderiyoruz
	transmitData(transmitIrData);
	endTranmitData();
}

void mainProccessCenter(){
	  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_10,GPIO_PIN_SET); //Mavi led
	  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_11,GPIO_PIN_RESET);//Sarý Led
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



