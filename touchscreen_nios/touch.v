module touch (
  input CLOCK_50,
  input [3:0] KEY,
  inout [35:0] GPIO_0,
  inout [35:0] GPIO_1,

  output [12:0] DRAM_ADDR,
  output [1:0] DRAM_BA,
  output DRAM_CAS_N,
  output DRAM_CKE,
  output DRAM_CS_N,
  output DRAM_RAS_N,
  output DRAM_WE_N,
  inout [15:0] DRAM_DQ,
  output DRAM_UDQM,
  output DRAM_LDQM,
  output VGA_HS,
  output VGA_VS,
  output [7:0] VGA_R,
  output [7:0] VGA_G,
  output [7:0] VGA_B,
  output DRAM_CLK,
  output VGA_CLK,
  output VGA_BLANK_N,
  output VGA_SYNC_N
);

  nios nios_u0 (
    .clk_clk          (CLOCK_50),
    .reset_reset_n    (KEY[0]),
    .touchscreen_rxd  (GPIO_0[11]),
    .touchscreen_txd  (GPIO_0[10]),
    .wifi_module_RXD  (GPIO_1[33]),   // wifi_module.RXD
    .wifi_module_TXD  (GPIO_1[35]),   //            .TXD 
    
    .sdram_wire_addr  (DRAM_ADDR),  //  sdram_wire.addr
    .sdram_wire_ba    (DRAM_BA),    //            .ba
    .sdram_wire_cas_n (DRAM_CAS_N), //            .cas_n
    .sdram_wire_cke   (DRAM_CKE),   //            .cke
    .sdram_wire_cs_n  (DRAM_CS_N),  //            .cs_n
    .sdram_wire_dq    (DRAM_DQ),    //            .dq
    .sdram_wire_dqm   ({DRAM_UDQM, DRAM_LDQM}),   //            .dqm
    .sdram_wire_ras_n (DRAM_RAS_N), //            .ras_n
    .sdram_wire_we_n  (DRAM_WE_N),  //            .we_n
    .sdram_clk_clk    (DRAM_CLK),    //   sdram_clk.clk
    .vga_out_CLK      (VGA_CLK),      //     vga_out.CLK
    .vga_out_HS       (VGA_HS),       //            .HS
    .vga_out_VS       (VGA_VS),       //            .VS
    .vga_out_BLANK    (VGA_BLANK_N),    //            .BLANK
    .vga_out_SYNC     (VGA_SYNC_N),     //            .SYNC
    .vga_out_R        (VGA_R),        //            .R
    .vga_out_G        (VGA_G),        //            .G
    .vga_out_B        (VGA_B)         //            .B
  );

endmodule