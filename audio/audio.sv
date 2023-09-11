module wifi_module (input logic CLOCK_50, input logic [3:0] KEY,
                    input logic [9:0] SW, output logic [9:0] LEDR,
                    input logic 
                    output logic [6:0] HEX0, output logic [6:0] HEX1, output logic [6:0] HEX2,
                    output logic [6:0] HEX3, output logic [6:0] HEX4, output logic [6:0] HEX5,
                    inout logic [35:0] GPIO_1
                    inout logic AUD_ADCLRCK, inout logic AUD_DACLRCK, audio_BCLK);

  input  logic  AUD_ADCDAT,      //        audio.ADCDAT
		input  logic  AUD_ADCLRCK,     //             .ADCLRCK
		input  logic  AUD_BCLK,        //             .BCLK
		output logic  AUD_DACDAT,      //             .DACDAT
		input  logic  AUD_DACLRCK,     //             .DACLRCK
		inout  logic  ADC_SDAT, // audio_config.SDAT
    
		output logic  ADC_SCLK,

  	nios_system sys (
		.clk_clk           (CLOCK_50),           //          clk.clk
		.reset_reset_n     (KEY[0])),     //        reset.reset_n
		.audio_ADCDAT      (<connected-to-audio_ADCDAT>),      //        audio.ADCDAT
		.audio_ADCLRCK     (<connected-to-audio_ADCLRCK>),     //             .ADCLRCK
		.audio_BCLK        (<connected-to-audio_BCLK>),        //             .BCLK
		.audio_DACDAT      (<connected-to-audio_DACDAT>),      //             .DACDAT
		.audio_DACLRCK     (<connected-to-audio_DACLRCK>),     //             .DACLRCK
		.audio_config_SDAT (<connected-to-audio_config_SDAT>), // audio_config.SDAT
		.audio_config_SCLK (<connected-to-audio_config_SCLK>)  //             .SCLK
	);
endmodule