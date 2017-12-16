#include "mbed.h"

#include "rfmesh.h"
#include "protocol.h"
#include "utils.h"


//------------------------------------- CONFIG -----------------------------------------
#define FLASH_HEADER	0x0800FFF0
//RF
#define F_NODEID	*(uint8_t *) FLASH_HEADER
#define F_CHANNEL	*(uint8_t *) (FLASH_HEADER+0x01)
//RGB LED
#define USE_RGB_LED 1
//APDS9960 (Colorlight sensor, gesture)
#define USE_APDS_SENSOR 1
#define USE_APDS_GESTURE 0
#define USE_APDS_LIGHT 1
//--------------------------------------------------------------------------------------
//TODO should have a board as a target
#define RF_BOARD_DONGLE 1
#define RF_BOARD_PIO 	0
//--------------------------------------------------------------------------------------
Serial   rasp(PB_10, PB_11, 115200);

#if(USE_RGB_LED == 1)
	#include "ws2812B.h"
	ws2812B rgb_led(PB_13);
#endif

#if(USE_APDS_SENSOR == 1)
	#include "glibr.h"
	glibr gsensor(PB_7,PB_6);
#endif

#if (RF_BOARD_DONGLE == 1)
	uint8_t spi_module = 1;
	//nRF Modules 1:Gnd, 2:3.3v, 3:ce,  4:csn, 5:sck, 6:mosi, 7:miso, 8:irq 
	RfMesh hsm(&rasp,spi_module,           PC_15, PA_4, PA_5,   PA_7,  PA_6,    PA_0);
#elif (RF_BOARD_PIO == 1)
uint8_t spi_module = 2;
#endif


DigitalOut debug_pio(PB_13);

Proto    prf(&rasp);
DigitalOut myled(PC_13);
Ticker tick_call;
uint8_t led_count = 0;

uint8_t payload[32];

bool is_rgb_toSend = false;
bool is_heat_toSend = false;
bool is_msg_toSend = false;
uint8_t msg_size = 0;
uint8_t tab_send[32];


void the_ticker()
{
	if(led_count != 0)
	{
		myled = 0;
		led_count--;
	}
	else
	{
		myled = 1;
	}
}

void rf_sniffed(uint8_t *data,uint8_t size)
{
	led_count = 1;
}

void rf_message(uint8_t *data,uint8_t size)
{
	if(data[rf::ind::pid] == rf::pid::rgb)
	{
		uint8_t red = data[5];
		uint8_t green = data[6];
		uint8_t blue = data[7];
		rgb_led.set(red,green,blue);
		rasp.printf("R:%u;G:%u;B:%u\r\n",red,green,blue);
	}
}

void init()
{
	uint8_t * p_UID = (uint8_t*) 0x1FFFF7E8;
	
	rasp.printf("stm32_bridge> U_ID: ");
	print_tab(&rasp,p_UID,12);
	rasp.printf("stm32_bridge> Node ID: %d\r",F_NODEID);

	tick_call.attach(&the_ticker,0.1);

	hsm.init(F_CHANNEL);//left to the user for more flexibility on memory management

    hsm.attach(&rf_sniffed,RfMesh::CallbackType::Sniff);
	hsm.attach(&rf_message,RfMesh::CallbackType::Message);

	hsm.setBridgeMode();
	rasp.printf("stm32_bridge> listening to Mesh 2.0 on channel %d in bridge Mode\n",F_CHANNEL);

	hsm.setNodeId(F_NODEID);


	hsm.setRetries(10);
	hsm.setAckDelay(400);
	
	//hsm.print_nrf();

	#if(USE_APDS_SENSOR == 1)
		if ( gsensor.ginit() ) {
			rasp.printf("apds> APDS-9960 initialization complete\n\r");
		} else {
			rasp.printf("apds> Something went wrong during APDS-9960 init\n\r");
		}
	#endif
	#if(USE_APDS_GESTURE == 1)
		// Start running the APDS-9960 gesture sensor engine
		if ( gsensor.enableGestureSensor(true) )
		{
			rasp.printf("apds> Gesture sensor is now running\n\r");
		} else 
		{
			rasp.printf("apds> Something went wrong during gesture sensor init!\n\r");
		}
	#endif
	#if(USE_APDS_LIGHT == 1)
		if ( gsensor.enableLightSensor() ) 
		{
			rasp.printf("apds> Light sensor is on\n\r");
		} else 
		{
			rasp.printf("apds> Something went wrong during light sensor init!\n\r");
		}
	#endif

}

#if(USE_RGB_LED == 1)
void test_RGB()
{
        rgb_led.set(0xFF,0x00,0x00);
        wait(0.3);
        rgb_led.set(0x00,0xFF,0x00);
        wait(0.3);
        rgb_led.set(0x00,0x00,0xFF);
        wait(0.3);
        rgb_led.set(0x00,0x00,0x00);
}
#endif

#if(USE_APDS_LIGHT == 1)
void log_light_colors()
{
	uint16_t light_rgb[4];
	gsensor.readAmbientLight(light_rgb[0]);
	gsensor.readRedLight(light_rgb[1]);
	gsensor.readGreenLight(light_rgb[2]);
	gsensor.readBlueLight(light_rgb[3]);
	hsm.broadcast_light_rgb(light_rgb);
	//expected at rx gateway side
	rasp.printf("NodeId:%u;light:%u;red:%u,green:%u;blue:%u\r\n",F_NODEID,light_rgb[0],light_rgb[1],light_rgb[2],light_rgb[3]);
}
#endif

int main() 
{

	init();

	test_RGB();

	hsm.broadcast(rf::pid::reset);
    
	uint16_t alive_count = 520;
	uint16_t light_count = 86;// ~ 10 s

    while(1) 
    {
		wait_ms(100);
		if(hsm.nRFIrq.read() == 0)
		{
			rasp.printf("irq pin Low, missed interrupt, re init()\n");
			hsm.init(F_CHANNEL);
		}
		if(light_count == 0)
		{
			#if(USE_APDS_LIGHT == 1)
			log_light_colors();
			#endif
			light_count = 86;//~10 sec
		}
		else
		{
			light_count--;
		}
		if(alive_count == 0)
		{
			hsm.broadcast(rf::pid::alive);
			rasp.printf("NodeId:%u;status:Alive\r\n",F_NODEID);//expected at rx gateway side
			alive_count = 520;//~60 sec
		}
		else
		{
			alive_count--;
		}
	}
}
