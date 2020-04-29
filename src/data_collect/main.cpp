#include "mbed.h"

#include "fsl_port.h"

#include "fsl_gpio.h"


#define UINT14_MAX        16383

// FXOS8700CQ I2C address

#define FXOS8700CQ_SLAVE_ADDR0 (0x1E<<1) // with pins SA0=0, SA1=0

#define FXOS8700CQ_SLAVE_ADDR1 (0x1D<<1) // with pins SA0=1, SA1=0

#define FXOS8700CQ_SLAVE_ADDR2 (0x1C<<1) // with pins SA0=0, SA1=1

#define FXOS8700CQ_SLAVE_ADDR3 (0x1F<<1) // with pins SA0=1, SA1=1

// FXOS8700CQ internal register addresses

#define FXOS8700Q_STATUS 0x00

#define FXOS8700Q_OUT_X_MSB 0x01

#define FXOS8700Q_OUT_Y_MSB 0x03

#define FXOS8700Q_OUT_Z_MSB 0x05

#define FXOS8700Q_M_OUT_X_MSB 0x33

#define FXOS8700Q_M_OUT_Y_MSB 0x35

#define FXOS8700Q_M_OUT_Z_MSB 0x37

#define FXOS8700Q_WHOAMI 0x0D

#define FXOS8700Q_XYZ_DATA_CFG 0x0E

#define FXOS8700Q_CTRL_REG1 0x2A

#define FXOS8700Q_M_CTRL_REG1 0x5B

#define FXOS8700Q_M_CTRL_REG2 0x5C

#define FXOS8700Q_WHOAMI_VAL 0xC7


I2C i2c( PTD9,PTD8);

Serial pc(USBTX, USBRX);

float x_rec[100], y_rec[100], z_rec[100], a[100], q[100];

int index;

DigitalOut led3(LED3);

Ticker time_up;


Timer debounce;

InterruptIn btnRecord(SW2);

InterruptIn btnFlag(SW3);

EventQueue queue(32 * EVENTS_EVENT_SIZE);

EventQueue queue2(32 * EVENTS_EVENT_SIZE);

Thread t;
Thread t2;


int m_addr = FXOS8700CQ_SLAVE_ADDR1;

int idR[32] = {0};

int indexR = 0;


void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len) {

   char tmp = addr;

   i2c.write(m_addr, &tmp, 1, true);

   i2c.read(m_addr, (char *)data, len);

}


void FXOS8700CQ_writeRegs(uint8_t * data, int len) {

   i2c.write(m_addr, (char *)data, len);

}

void led1_thread() {

    while (index < 100) {

        led3 = !led3;
        wait(0.1);

    }

}

void record(void) {

   float x=0.0, y=0.0, z=0.0;

   

   int16_t acc16;

   uint8_t res[6];

   queue2.call(led1_thread);


   FXOS8700CQ_readRegs(FXOS8700Q_OUT_X_MSB, res, 6);

   acc16 = (res[0] << 6) | (res[1] >> 2);

   if (acc16 > UINT14_MAX/2)

     acc16 -= UINT14_MAX;

   x = ((float)acc16) / 4096.0f;

   acc16 = (res[2] << 6) | (res[3] >> 2);

   if (acc16 > UINT14_MAX/2)

     acc16 -= UINT14_MAX;

   y = ((float)acc16) / 4096.0f;

   acc16 = (res[4] << 6) | (res[5] >> 2);

   if (acc16 > UINT14_MAX/2)

     acc16 -= UINT14_MAX;

   z = ((float)acc16) / 4096.0f;
   
   //printf("%f, %f, %f\n", x * 1000.0, y * 1000.0, z * 1000.0);
   x_rec[index] = x; 
   y_rec[index] = y;
   z_rec[index] = z;
   index++;
   wait(0.1);
   a[index] = (x * x + y * y) ^ (1/2) * 0.1 * 0.1 / 2;
   q[index] = 9.8 * a;

}


void startRecord(void) {

  printf("---start---\n");

  index = 0;
  
  while (i < 100) {
    idR[indexR++] = queue.call_every(1,record);

    indexR = indexR % 32;
  }
}


void stopRecord(void) {
  int j;
  if (index == 100) {
    printf("---stop---\n");

    for (auto &i : idR)

        queue.cancel(i);
    for (j = 0; j < 100; j++) {
        printf("%f, %f, %f\n", 
        x_rec[j] * 1000.0, y_rec[j] * 1000.0, z_rec[j] * 1000.0);
        wait(0.1);
    }

  }
}


void flagWrong(void) {printf("---delete---\n");}


void initFXOS8700Q(void) {

  uint8_t data[2];

  FXOS8700CQ_readRegs( FXOS8700Q_CTRL_REG1, &data[1], 1);

  data[1] |= 0x01;

  data[0] = FXOS8700Q_CTRL_REG1;

  FXOS8700CQ_writeRegs(data, 2);

}


int main() {

  initFXOS8700Q();

  t.start(callback(&queue, &EventQueue::dispatch_forever));
  t.start(callback(&queue2, &EventQueue::dispatch_forever));

  btnRecord.fall(queue.event(startRecord));


  btnFlag.fall(queue.event(flagWrong));

  btnRecord.rise(queue.event(stopRecord));

}