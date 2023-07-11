#include "mbed.h"

I2C i2c(P0_0,P0_1);    //SDA,SCL oled and adc1
I2C i2c2(P0_10,P0_11);   //SDA,SCL adc2
DigitalOut rc1(P2_1);   //relay1
DigitalOut rc2(P2_0);   //relay2
DigitalIn sw1(P0_4);    //sw1
DigitalIn sw2(P0_5);    //sw2

//OLED
const uint8_t oled1=0x78;   //oled i2c addr 0x3c<<1
void oled_init(uint8_t addr);     //lcd init func
void char_disp(uint8_t addr, uint8_t position, char data);    //char disp func
void cont(uint8_t addr,uint8_t val);     //contrast set
void vm_disp(uint8_t addr, uint8_t position, float val);
void im_disp(uint8_t addr, uint8_t position, float val);

//ADC settings
#define adc_res 32768   //2^15 adc resolution
#define adc_ref 2048    //2.048V adc reference
#define att 0.05838       //volt attenuation
#define sens_R 0.1      //sense R (ohm)
#define vm1_cal 1.005023    //vm1 calibration
#define im1_cal 1.002355    //im1 calibration
#define vm2_cal 0.998752    //vm1 calibration
#define im2_cal 1.000350    //im1 calibration
const uint8_t adc=0xd0;     //adc addr
const uint8_t sps=0b10;     //0b00->240sps,12bit 0b01->60sps,14bit 0b10->15sps,16bit
char buf[2], set1[1], set2[1];  //i2c buf, adc setting val
uint8_t pga1=0,pga2=0;          //0->x1 1->x2 2->x4 3->x8
int16_t raw_val;      //adc raw val
float val;            //float val mV/mA unit
uint8_t i;
int32_t total[2];
const uint8_t avg=10;

int main(){
    i2c.frequency(100000);  //I2C clk 100kHz
    
    //OLED init
    thread_sleep_for(50);  //wait for OLED power on
    oled_init(oled1);
    thread_sleep_for(10);
    cont(oled1,0xff);

    char_disp(oled1,0,'C');
    char_disp(oled1,1,'H');
    char_disp(oled1,2,'1');
    char_disp(oled1,3,':');
    char_disp(oled1,0x20,'C');
    char_disp(oled1,0x21,'H');
    char_disp(oled1,0x22,'2');
    char_disp(oled1,0x23,':');

    set1[0]=(0b1001<<4)+(sps<<2)+pga1;
    i2c.write(adc,set1,1);
    set2[0]=(0b1001<<4)+(sps<<2)+pga2;
    i2c2.write(adc,set2,1);

    char_disp(oled1,13,'V');
    char_disp(oled1,0x20+13,'V');

    while (true){
        //adc1
        if(sw1==1){     //VM
            rc1=0;      //relay off
            pga1=0;
            if(set1[0]!=(0b1001<<4)+(sps<<2)+pga1){
                set1[0]=(0b1001<<4)+(sps<<2)+pga1;
                i2c.write(adc,set1,1);
                char_disp(oled1,12,' ');
                char_disp(oled1,13,'V');
            }
        }else{  //IM
            rc1=1;      //relay on
            pga1=3;
            if(set1[0]!=(0b1001<<4)+(sps<<2)+pga1){
                set1[0]=(0b1001<<4)+(sps<<2)+pga1;
                i2c.write(adc,set1,1);
                char_disp(oled1,11,' ');
                char_disp(oled1,12,'A');
                char_disp(oled1,13,' ');
            }
        }
        //adc2
        if(sw2==1){     //VM
            rc2=0;      //relay off
            pga2=0;
            if(set2[0]!=(0b1001<<4)+(sps<<2)+pga2){
                set2[0]=(0b1001<<4)+(sps<<2)+pga2;
                i2c2.write(adc,set2,1);
                char_disp(oled1,0x20+12,' ');
                char_disp(oled1,0x20+13,'V');
            }
        }else{  //IM
            rc2=1;      //relay on
            pga2=3;
            if(set2[0]!=(0b1001<<4)+(sps<<2)+pga2){
                set2[0]=(0b1001<<4)+(sps<<2)+pga2;
                i2c2.write(adc,set2,1);
                char_disp(oled1,0x20+11,' ');
                char_disp(oled1,0x20+12,'A');
                char_disp(oled1,0x20+13,' ');
            }
        }
        
        //adc read
        for (i=0;i<avg;++i){
                i2c.read(adc|1,buf,2);  //adc1 read
                raw_val=(buf[0]<<8)+buf[1];
                total[0]=total[0]+(int32_t)raw_val;
                i2c2.read(adc|1,buf,2);  //adc1 read
                raw_val=(buf[0]<<8)+buf[1];
                total[1]=total[1]+(int32_t)raw_val;
                thread_sleep_for(20);
        }

        //disp1 val
        if(sw1==1){     //vm
            val=(float)total[0]/avg*adc_ref/adc_res/att*vm1_cal;     //mV unit
            vm_disp(oled1,5,val);
        }else{
            val=(float)total[0]/avg*adc_ref/adc_res/8/sens_R*im1_cal;     //mA unit expression
            im_disp(oled1,5,val);
        }
        //disp2 val
        if(sw2==1){     //vm
            val=(float)total[1]/avg*adc_ref/adc_res/att*vm2_cal;     //mV unit
            vm_disp(oled1,0x20+5,val);
        }else{
            val=(float)total[1]/avg*adc_ref/adc_res/8/sens_R*im2_cal;     //mA unit expression
            im_disp(oled1,0x20+5,val);
        }

        //adc1 overflow check
        if(abs(total[0]/avg)>=32767){
            char_disp(oled1,0xf,'!');
        }else{
            char_disp(oled1,0xf,' ');
        }
        //adc2 overflow check
        if(abs(total[1]/avg)>=32767){
            char_disp(oled1,0x20+0xf,'!');
        }else{
            char_disp(oled1,0x20+0xf,' ');
        }
        total[0]=0;
        total[1]=0;
    }
}

//oled init func
void oled_init(uint8_t addr){
    char oled_data[2];
    oled_data[0] = 0x0;
    oled_data[1]=0x01;           //0x01 clear disp
    i2c.write(addr, oled_data, 2);
    thread_sleep_for(20);
    oled_data[1]=0x02;           //0x02 return home
    i2c.write(addr, oled_data, 2);
    thread_sleep_for(20);
    oled_data[1]=0x0C;           //0x0c disp on
    i2c.write(addr, oled_data, 2);
    thread_sleep_for(20);
    oled_data[1]=0x01;           //0x01 clear disp
    i2c.write(addr, oled_data, 2);
    thread_sleep_for(20);
}

void char_disp(uint8_t addr, uint8_t position, char data){
    char buf[2];
    buf[0]=0x0;
    buf[1]=0x80+position;   //set cusor position (0x80 means cursor set cmd)
    i2c.write(addr,buf, 2);
    buf[0]=0x40;            //ahr disp cmd
    buf[1]=data;
    i2c.write(addr,buf, 2);
}

//set contrast func
void cont(uint8_t addr,uint8_t val){
    char buf[2];
    buf[0]=0x0;
    buf[1]=0x2a;
    i2c.write(addr,buf,2);
    buf[1]=0x79;    //SD=1
    i2c.write(addr,buf,2);
    buf[1]=0x81;    //contrast set
    i2c.write(addr,buf,2);
    buf[1]=val;    //contrast value
    i2c.write(addr,buf,2);
    buf[1]=0x78;    //SD=0
    i2c.write(addr,buf,2);
    buf[1]=0x28;    //0x2C, 0x28
    i2c.write(addr,buf,2);
}

void vm_disp(uint8_t addr, uint8_t position, float val){
    char buf[8];
    int32_t val_int;
    buf[0]=0x0;
    buf[1]=0x80+position;   //set cusor position (0x80 means cursor set cmd)
    i2c.write(addr,buf,2);
    buf[0]=0x40;        //write cmd
    val_int=(int16_t)val;
    if(val_int>=0)buf[1]=0x2B;      //+
    else{
        buf[1]=0x2D;    //-
        val_int=abs(val_int);
    }
    buf[2]=0x30+(val_int/10000)%10;//10
    buf[3]=0x30+(val_int/1000)%10; //1
    buf[4]=0x2E;                   //.
    buf[5]=0x30+(val_int/100)%10;  //0.1
    buf[6]=0x30+(val_int/10)%10;   //0.01
    buf[7]=0x30+val_int%10;        //0.001
    i2c.write(addr,buf,8);
}

void im_disp(uint8_t addr, uint8_t position, float val){
    char buf[7];
    int16_t val_int;
    buf[0]=0x0;
    buf[1]=0x80+position;   //set cusor position (0x80 means cursor set cmd)
    i2c.write(addr,buf,2);
    buf[0]=0x40;        //write cmd
    val_int=(int16_t)val;
    if(val_int>=0)buf[1]=0x2B;      //+
    else{
        buf[1]=0x2D;    //-
        val_int=abs(val_int);
    }
    buf[2]=0x30+(val_int/1000)%10; //1
    buf[3]=0x2E;                   //.
    buf[4]=0x30+(val_int/100)%10;  //0.1
    buf[5]=0x30+(val_int/10)%10;   //0.01
    buf[6]=0x30+val_int%10;        //0.001
    i2c.write(addr,buf,7);
}