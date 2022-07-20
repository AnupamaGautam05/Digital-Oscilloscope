/*! \mainpage Week-4 Challenge Activity 2: Design Digital Oscilloscope
 *
 * @author     e-Yantra Team
 * @date       2021/06/07
 *
 * \subsection Aim
 * To display waveforms from function generator on GLCD with switches representing Digital Oscilloscope (DSO).
 *
 * \subsection Connections
 * GLCD Connections:							<br>
 *		 LCD Pins	  Micro-controller Pins     <br>
 *			  RS  --> PB0						<br>
 *			  RW  --> PB2						<br>
 *			  EN  --> PB1						<br>
 *			  DB7 --> PD7						<br>
 *			  DB6 --> PD6						<br>
 *			  DB5 --> PD5						<br>
 *			  DB4 --> PD4						<br>
 *
 * The DSO screen has total size of 128 x 64 pixels, which is divided into two sections:
 * 
 * [ I ] DISPLAY WAVEFORM => Area where the input waveform is to be displayed (may it be Sine, Square, etc.)
 * 							 The center line is a reference line about which the waveform should be symmetric.
 * 							 The height of this area should accomodate the waveform with max. Vpp of 5V.
 * 							 The width of this area should accomodate the waveform with:
 * 								* min. frequency of 250 Hz (0.25 kHz)
 * 								* max. frequency of 2000 Hz (2 kHz)
 * 
 * [ II ] DISPLAY WAVEFORM INFO => Area where the information about the waveform is to be displayed.
 * 								   The peak-to-peak voltage (Vpp), frequency in kHz, ms/div, V/div.
 * 
 *  ____________________________________________________________
 * |(0,0)                                   |                   |
 * |                                        |                   |
 * |                                        |                   |
 * |          DISPLAY WAVEFORM              |                   |
 * |                                        |                   |
 * |                                        |     DISPLAY       |
 * |                                        |                   |
 * |________________________________________|                   |
 * |                                        |     WAVEFORM      |
 * |                                        |                   |
 * |                                        |                   |
 * |          DISPLAY WAVEFORM              |       INFO        |
 * |                                        |                   |
 * |                                        |                   |
 * |                                        |                   |
 * |_________________________________(95,63)|(96,63)____(127,63)|
 */

//---------------------------------- HEADER FILES -----------------------------------------------------
#include "firebird_simulation.h"
#include "u8glib.h"
#include "adc.h"
#include <util/delay.h>

//------------------------------------- MACROS ----------------------------------------------------------
// If any

//---------------------------------- GLOBAL VARIABLES ---------------------------------------------------
// If any

// array to store the data from the input waveform
// since the waveform display area is of 96 columns or pixels, we store double amount of data in buffer
// Note: the input waveform from the Function Generator has minimum value of 0 V
int buffer_input_data[192];

// values from buffer array for max., min. and half of the amplitude
int V_max, V_min, V_mid;

// peak-to-peak voltage of the input waveform
float V_pp;

// index values from buffer array which indicate:
// start of waveform : where the first wave in the buffer cuts half of the amplitude or where the first wave starts its cycle
// end of waveform : where the second wave in the buffer cuts half of the amplitude or where the first wave ends its cycle or the second wave starts its cycle
int start_wave_idx, end_wave_idx;

// msec per division to be considered in the "Display Waveform" Area
float ms_per_div = 0.5;

// volts per division to be considered in the "Display Waveform" Area
float volts_per_div = 0.625;

// frequency of input waveform in kHz unit
float freq;

// array to compute and store the pixel locations for the input waveform to be displayed in "Display Waveform" Area
int pixel_loc[96];

// << NOTE >> : You can define any extra global variables below
int hold_flag = 0;
int vert_disp = 0;


//---------------------------------- FUNCTIONS ----------------------------------------------------------
// << TODO >> : Complete all the functions as per the instructions given in form of comments

/**
 * @brief      Makes **ONLY** DIV+, DIV-, HOLD, UP, DOWN switch pins and dso_input_channel pin as input,
 * 				Activates pull-up for **ONLY** these switch pins and deactivates pull-up for **ONLY** dso_input_channel pin
 */
void dso_switches_input_channel_config(void)
{
	// << NOTE >> : Use Masking and Shift Operators here
	// << TODO >> : Complete the function as expected in the comment above.
	
	// Make **ONLY** three switches (DIV+, DIV- and HOLD) connected to div_hold_switch_port (PORTD) as input
	div_hold_switch_ddr_reg &= ~((1<<div_add_sw_pin)|(1<<hold_sw_pin)|(1<<div_sub_sw_pin)) ;
	
	// Activate pull-up for **ONLY** for three switches (DIV+, DIV- and HOLD) connected to div_hold_switch_port (PORTD)
	div_hold_switch_port_reg |= ((1<<div_add_sw_pin)|(1<<hold_sw_pin)|(1<<div_sub_sw_pin)) ;

	// Make **ONLY** two switches (UP and DOWN) connected to up_down_switch_port (PORTE) as input
	up_down_switch_ddr_reg &= ~((1<<up_sw_pin)|(1<<down_sw_pin)) ;

	// Activate pull-up for **ONLY** for two switches (UP and DOWN) connected to up_down_switch_port (PORTE)
	up_down_switch_port_reg |=((1<<up_sw_pin)|(1<<down_sw_pin));

	// Make **ONLY** the dso_input_channel pin connected to dso_input_channel_port (PORTK) as input
	dso_input_channel_ddr_reg &= ~(1<<dso_input_channel_pin);

	// Deactivate pull-up for **ONLY** dso_input_channel pin connected to dso_input_channel_port (PORTK)
	dso_input_channel_port_reg &=~(1<<dso_input_channel_pin) ;
}


/**
 * @brief      Configures External Interrupt on DIV+, DIV-, HOLD, UP, DOWN switch pins
 */
void dso_switches_interrupt_config(void)
{
	// << NOTE >> : Use Masking and Shift Operators here
	// << TODO >> : Complete the function as expected in the comment above

	// All interrupts have to be disabled before configuring interrupts
	cli();	// Disable interrupts globally

	// Configure falling edge detection on div_add_sw_pin_int (INT1), div_sub_sw_pin_int (INT2) and hold_sw_pin_int (INT3)
	EICRA_reg |=((1<<div_add_sw_ISC_bit1)|(1<<div_sub_sw_ISC_bit1)|(1<<hold_sw_ISC_bit1));
	EICRA_reg &=((1<<div_add_sw_ISC_bit0)|(1<<div_sub_sw_ISC_bit0)|(1<<hold_sw_ISC_bit0)) ;

	// Configure falling edge detection on up_sw_pin_int (INT4) and down_sw_pin_int (INT5)
	EICRB_reg |=((1<<up_sw_ISC_bit1)|(1<<down_sw_ISC_bit1)) ;
	EICRB_reg &=((1<<up_sw_ISC_bit0)|(1<<down_sw_ISC_bit0)) ;

	// Enable INT1, INT2, INT3, INT4 and INT5 interrupts
	EIMSK_reg |=((1<<div_add_sw_pin_int)|(1<<div_sub_sw_pin_int)|(1<<hold_sw_pin_int)|(1<<up_sw_pin_int)|(1<<down_sw_pin_int)) ;

	sei();	// Enable interrupts globally
}


/**
 * @brief      Initializes the Digital Oscilloscope peripherals (switches, display, input channel)
 */
void dso_init(void)
{
	// << NOTE >> : You are not allowed to modify or change anything inside this function

	dso_switches_input_channel_config();
	dso_switches_interrupt_config();
	adc_init();
	glcd_setup();
}


/**
 * @brief      Interrupt Service Routine for div_add_sw_pin_int (INT1)
 */
ISR(div_add_sw_pin_int_vect)
{   
	ms_per_div=ms_per_div+0.5;
	if (ms_per_div>=2)
	ms_per_div=2;
	
}


/**
 * @brief      Interrupt Service Routine for div_sub_sw_pin_int (INT2)
 */
ISR(div_sub_sw_pin_int_vect)
{    
	
	ms_per_div=ms_per_div-0.5;
	if (ms_per_div<=0.125)
	ms_per_div=0.125;
	
}


/**
 * @brief      Interrupt Service Routine for hold_sw_pin_int (INT3)
 */
ISR(hold_sw_pin_int_vect)
{
	hold_flag ^= hold_flag;
}


/**
 * @brief      Interrupt Service Routine for up_sw_pin_int (INT4)
 */
ISR(up_sw_pin_int_vect)
{
	vert_disp--;
	if (vert_disp<=-31)
	vert_disp=-30;
	
}


/**
 * @brief      Interrupt Service Routine for down_sw_pin_int (INT5)
 */
ISR(down_sw_pin_int_vect)
{
	vert_disp++;
	if (vert_disp>=31)
	vert_disp=30;
	
}


/**
 * @brief      Display the Start Screen of DSO
 */
void dso_start_screen(void)
{
	setFont(u8g_font_5x7);
	firstPage();

	// << NOTE >> : You can add your creativity over here!
	do
	{
		drawStr( 32, 32, "DSO START");
		
	}
	while( nextPage() );
}


/**
 * @brief      Sample the waveform from the DSO input channel and store the data in a buffer array.
 */
void sample_input_wave(void)
{
	// << NOTE >> : You can add your creativity over here!
	// << TODO >> : Complete the logic in function as expected in the comment above.

	for (int i = 0; i < 192; i++)
	{
		// store the data from input waveform into buffer
		buffer_input_data[i] = convert_analog_channel_data(dso_input_channel_pin);
		
		// sample the data as required by giving small delays in capturing the input data
		
	}
}


/**
 * @brief      From the buffer data of input waveform, calculate the values in buffer array which is max. (V_max), min. (V_min),
 * 			   half (V_mid) of the amplitude and the peak-to-peak voltage (V_pp) in Volts of the given waveform
 */
void calc_vpp_vmid(void)
{
	// << TODO >> : Complete the logic in function as expected in the comment above.

	V_max = 0;
	V_min = 0;

	// calculate these values:
	// V_max : value in buffer array that is maximum of the amplitude
	// V_min : value in buffer array that is minimum of the amplitude
	for (int i = 0; i < 192; i++)
	{
		if (buffer_input_data[i] > V_max)
			V_max = buffer_input_data[i];
		
		if (buffer_input_data[i] < V_min)
			V_min = buffer_input_data[i];
	}

	// calculate V_mid : value in buffer array that is half of the amplitude
	V_mid = (V_max-V_min)/2;

	// calculate V_pp : value in Volts for the peak-to-peak amplitude of the input waveform
	V_pp = ((V_max - V_min) * 5.00) / 255 ;				//((V_max - V_min) * 5.00) / 255
}


/**
 * @brief      Calculate the index values from buffer array where the "first" wave in given waveform starts (start_wave_idx) and ends (end_wave_idx) its cycle
 *		     _______	    ________
 *          |		|	   |		|
 *  ___start|_______|___end|________|______
 *          |		|	   |		|
 *          |		|______|		|
 */
void calc_start_end_wave_idx(void)
{
	// << TODO >> : Complete the logic in function as expected in the comment above.

	start_wave_idx = 0;
	end_wave_idx = 0;

	for (int i = 0; i < 97; i++)
	{
		if (buffer_input_data[i] < V_mid && buffer_input_data[i+1] >= V_mid)
		{
			start_wave_idx = i;
			break;
		}
	}

	for (int i = start_wave_idx + 1; i < 96 + start_wave_idx; i++)
	{
		if (buffer_input_data[i] < V_mid && buffer_input_data[i+1] >= V_mid)
		{
			end_wave_idx = i;
			break;
		}
	}
}


/**
 * @brief      Find the frequency of input waveform
 */
void calc_frequency(void)
{
	// << TODO >> : Complete the logic in function as expected in the comment above.
	float time = ( ( ( end_wave_idx - start_wave_idx) / 8) * ms_per_div);
	freq = 1.0/(time) ;
}


/**
 * @brief      Measure various parameters of input waveform such as V_pp in Volts, V_mid, start_wave_idx, end_wave_idx and freq in kHz
 */
void measure_parameters(void)
{
	// << NOTE >> : You are not allowed to modify or change anything inside this function

	// calculate the V_pp (peak-to-peak) voltage in Volts and the V_mid (value from buffer array which is half the amplitude)
	calc_vpp_vmid();

	// calculate the index values from buffer array where the "first" wave in given waveform starts (start_wave_idx) and ends (end_wave_idx) its cycle
	calc_start_end_wave_idx();

	// calculate the frequency of input waveform in kHz unit
	calc_frequency();
}


/**
 * @brief      Compute and store the pixel locations of the "Display Waveform" to show each data point of input waveform
 * 				or Mapping the data points of input waveform to the pixel locations inside "Display Waveform" area
 */
void translate_wave_to_pixels(void)
{
	// << TODO >> : Complete the logic in function as expected in the comment above.

	int start = 0;

	int ref_val = 32 + ( ( ( V_max + V_min ) / 2 ) >> 2);
	for (int i = 0; i < 96; i++)
	{
		pixel_loc[i] = ref_val - (buffer_input_data[i+start] >> 2);
	}
}


/**
 * @brief      Draw the reference frame in "Display Waveform" Area for easy view of the input waveform
 * 			   with equal divisions for Time in msec (on X-axis) and Voltage level in Volts (on Y-axis)
 */
void draw_ref_frame(void)
{
	// << NOTE >> : You are not allowed to modify or change anything inside this function

	// drawing outer boundary or rectangle in the "Display Waveform" Area of 96 (columns) x 64 (rows)
	drawFrame(0, 0, 95, 63);

	// drawing the center horizontal and vertical line for reference and symmetricity
	drawLine(48, 0, 48, 63);
	drawLine(0, 32, 95, 32);

	// drawing short vertical lines or markings on center horizontal line with equal divisions for Time in msec
	for (int x = 0; x < 97; x += 8)
		drawLine(x, 31, x, 33);
	
	// drawing short horizontal lines or markings on center vertical line with equal divisions for Voltage level in Volts
	for (int y = 0 ; y < 64; y += 8)
		drawLine(47, y, 49, y);
	
	// plot small dots in all four quadrants for symmetricity in viewing the input waveform
	for (int x = 8; x < 96; x += 8)
	{
		for (int y=8; y < 64; y += 8)
			drawPixel(x,y);
	}
}


/**
 * @brief      Draw the input waveform data points translated to pixel locations in "Display Waveform" area
 * 			   Make use of "drawLine" function from u8g library to connect the pixel dots or data points so the input waveform is visualized
 */
void draw_waveform(void)
{
	// << TODO >> : Complete the logic in function as expected in the comment above.

	int y1, y2;

	for (int x = 0; x < 95; x++)
	{
		y1 = pixel_loc[x] + vert_disp;				// move the waveform up and down by +/- vert_disp
		y2 = pixel_loc[x + 1] + vert_disp;			// move the waveform up and down by +/- vert_disp

		drawLine(x, y1, x, y2);
	}
}


/**
 * @brief      Write text in the "Display Waveform Info" Area for displaying various parameters measured of the input waveform
 * 			   and data of parameters measured from the given waveform
 */
void display_wave_info_text_data(void)
{
	// << NOTE >> : You are not allowed to modify or change anything inside this function

	drawStr(96, 7, "ms/div");
	setPrintPos(96, 14);
	print_float(ms_per_div, 3);

	drawStr(96, 23, "V/div");
	setPrintPos(96, 30);
	print_float(volts_per_div, 3);
	
	drawStr(96, 40, "Vpp");
	setPrintPos(96, 47);
	print_float(V_pp, 3);
	drawStr(123, 47, "V");
	
	drawStr(96, 55, "f(kHz)");
	setPrintPos(96, 62);
	print_float(freq, 3);
}


/**
 * @brief      Display the input waveform in "Display Waveform" area; info text and measured parameter values in "Display Waveform Info" area
 */
void dso_display_waveform_data(void)
{
	// << NOTE >> : You are not allowed to modify or change the "do-while" loop
	// << TODO >> : You can although add any condition, if required for more features
	
	if (hold_flag == 0)
	{
		firstPage();
		do
		{
			// display the reference frame
			draw_ref_frame();

			// display the waveform as close and identical as possible with the input waveform by Function Generator
			draw_waveform();

			// display the waveform info text and data of measured parameters
			display_wave_info_text_data();
		}
		while( nextPage() );
	}
}


/**
 * @brief      Start the operation of DSO to sample, measure, translate and display the waveform and parameter values in the respective areas on GLCD
 */
void start_dso_operation(void)
{
	// << NOTE >> : You are not allowed to modify or change anything inside this function

	// sample the input waveform
	sample_input_wave();

	// measure various parameters of the given waveform
	measure_parameters();

	// convert the waveform data points to pixel coordinates of the GLCD
	translate_wave_to_pixels();

	// display the waveform and data of measured parameters
	dso_display_waveform_data();
}


//------------------------------------------- MAIN ------------------------------------------------------------
/**
 * @brief      Main Function
 *
 * @details    Initializes the DSO peripherals. Calls the Start Screen of DSO and initiates the DSO operation.
 */
int main(void)
{
	dso_init();
	dso_start_screen();

	while(1)
	{
		start_dso_operation();
	}
	return 0;
}
