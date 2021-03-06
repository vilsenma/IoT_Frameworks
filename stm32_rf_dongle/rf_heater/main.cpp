#include "mbed.h"

#include "rfmesh.h"
#include "utils.h"
#include "protocol.h"

//------------------------------------- CONFIG -----------------------------------------
#define FLASH_HEADER	0x0800FFF0
#define F_NODEID	*(uint8_t *) FLASH_HEADER
#define F_CHANNEL	*(uint8_t *) (FLASH_HEADER+0x01)
//--------------------------------------------------------------------------------------
#define RF_BOARD_DONGLE 1
#define RF_BOARD_PIO 	0
//--------------------------------------------------------------------------------------
Serial   rasp(PB_10, PB_11, 115200);

#if (RF_BOARD_DONGLE == 1)
	uint8_t spi_module = 1;
	//nRF Modules 1:Gnd, 2:3.3v, 3:ce,  4:csn, 5:sck, 6:mosi, 7:miso, 8:irq 
	RfMesh hsm(&rasp,spi_module,           PC_15, PA_4, PA_5,   PA_7,  PA_6,    PA_0);
#elif (RF_BOARD_PIO == 1)
uint8_t spi_module = 2;
#endif

DigitalOut myled(PC_13);
Ticker tick_call;

DigitalOut heater(PB_13);
const uint8_t HEAT_MAX = 10;

uint8_t heat_val = 0;
uint8_t tick_cycle = 0;
bool is_user_act = false;


void rf_message_to_me(uint8_t *data,uint8_t size)
{
	rasp.printf("RX(%d)> ",size);
	print_tab(&rasp,data,size);
	if(data[rf::ind::pid] == rf::pid::heat)
	{
		uint8_t heat_cmd = data[5];//heat_val payload : Size Control Pid  SrcId TrgId  HeatVal CRC
		if(heat_cmd == 11)
		{
			heat_val += 2;//Up go up quickly, significant step
			if(heat_val > 10)
			{
				heat_val = 10;
			}
		}
		else if(heat_cmd == 12)
		{
			if(heat_val > 0)
			{
				heat_val--;//decrease slowly
			}//else do nothing, already off
		}
		else if(heat_cmd <= 10)//direct value order
		{
			heat_val = heat_cmd;
		}

		tick_cycle = 0;//restart a new cycle for immidiate application
		rasp.printf("stm32_heater> (From RF) Set Heat Val to %d\r",heat_val);
		is_user_act = true;
	}

}

void the_ticker()
{
	static uint8_t l_heat_val = 0;

	if(tick_cycle == 0)
	{
		l_heat_val = heat_val;//cache it for teh next cycle

		if(l_heat_val != 0)
		{
			heater = 1;
		}
		else
		{
			heater = 0;//was forgotten !! important, as next is else only and avoid a tiny pulse in case of val of 0
		}
	}
	else if(tick_cycle == l_heat_val)
	{
		heater = 0;
	}

	myled = !myled;//alive

	//rasp.printf("tick %d ; heat_output %d ; heat val %d\r",tick_cycle,heater.read(),l_heat_val);

	tick_cycle++;
	if(tick_cycle == HEAT_MAX)
	{
		tick_cycle = 0;
	}
}

void init()
{
	heater = 0;
    rasp.printf("stm32_heater> Hello from the RF Heater\n");

    tick_call.attach(&the_ticker,1);

	hsm.init(F_CHANNEL);//left to the user for more flexibility on memory management
	hsm.nrf.setMode(nrf::Mode::Rx);//not set by default as to check power consemption with hci
	rasp.printf("mode:%d;channel:%d\n",hsm.nrf.getMode(),hsm.nrf.getChannel());

	hsm.setNodeId(F_NODEID);
	rasp.printf("stm32_heater> Started listening\n");

	hsm.setRetries(10);
	hsm.setAckDelay(400);
	
	//hsm.print_nrf();

	hsm.attach(&rf_message_to_me,RfMesh::CallbackType::Message);//matching the node id, safe wit hACK

}

const float one_minute = 60;

void long_wait(uint8_t min)
{
	//hsm.broadcast_byte(rf::pid::heat,heat_val);
	rasp.printf("stm32_heater> Level %u : for %u min\n",heat_val,min);
	wait(min * one_minute);
}

void pulse_heat(uint8_t val)
{
	heat_val = val;
	long_wait(5);//Level 10 for 5 min
	if(heat_val > 3 )heat_val-=3;
	long_wait(10);//l 7
	if(heat_val > 2 )heat_val-=2;
	long_wait(30);//l 5
	if(heat_val > 2 )heat_val-=3;
	long_wait(60);//l 2
}

int main() 
{

	uint8_t * p_UID = (uint8_t*) 0x1FFFF7E8;

	rasp.printf("stm32_heater> U_ID: ");
	print_tab(&rasp,p_UID,12);
	rasp.printf("stm32_heater> Node ID: %d\r",F_NODEID);
	

    init();

	hsm.print_nrf();

	hsm.broadcast(rf::pid::reset);
	wait(1);
	
	pulse_heat(10);//friendly to user increase decrease
	//after the run_heater_program, the heat_cal stays at 2

	//heat_val = 0;rasp.printf("stm32_heater> Program Over\r");

	while(1) 
    {
		if(heat_val > 2)
		{
			heat_val--;
		}
		long_wait(10);//here we wait 10 min
	}
}
