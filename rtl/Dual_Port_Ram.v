`include "common.vh"
module Dual_Port_RAM
#(
  parameter AWIDTH = 13, // address width
  parameter DWIDTH = 32 // data width
)
(
  input clka, // clock
  ///*** Port A***///
  input ena, // port A read enable
  input wea, // port A write enable
  input [AWIDTH-1:0] addra, // port A address
  input [DWIDTH-1:0] dina, // port A data
  output reg [DWIDTH-1:0] douta, // port A data output
  
  ///*** Port B***///
  input clkb, // clock
  input enb, // port A read enable
  input web, // port A write enable
  input [AWIDTH-1:0] addrb, // port A address
  input [DWIDTH-1:0] dinb, // port A data
  output reg [DWIDTH-1:0] doutb // port A data output
);

	reg [DWIDTH-1:0] mem [2**AWIDTH-1:0];

	always @(posedge clka) begin
		// /*** Port A***///
		if (ena&enb) begin
			if(wea) begin
				mem[addra] <= dina;
			end
			else if(web) begin
				mem[addrb] <= dinb;
			end
			
			douta <= mem[addra];
			doutb <= mem[addrb];
		end
		else if(ena) begin
			if(wea) begin
				mem[addra] <= dina;
			end
			douta <= mem[addra];
		end
		else if (enb) begin
			if(web) begin
				mem[addrb] <= dinb;
			end
			doutb <= mem[addrb];
		end
		
	end
	
endmodule

module Dual_Port_RAM_2
#(
  parameter AWIDTH = 10, // address width
  parameter DWIDTH = 32 // data width
)
(
  input clka, // clock
  ///*** Port A***///
  input ena, // port A read enable
  input wea, // port A write enable
  input [AWIDTH-1:0] addra, // port A address
  input [DWIDTH-1:0] dina, // port A data
  output reg [DWIDTH-1:0] douta, // port A data output
  
  ///*** Port B***///
  input clkb, // clock
  input enb, // port A read enable
  input web, // port A write enable
  input [AWIDTH-1:0] addrb, // port A address
  input [DWIDTH-1:0] dinb, // port A data
  output reg [DWIDTH-1:0] doutb // port A data output
);

	reg [DWIDTH-1:0] mem [2**AWIDTH-1:0];

	always @(posedge clka) begin
		// /*** Port A***///
		if (ena&enb) begin
			if(wea) begin
				mem[addra] <= dina;
			end
			else if(web) begin
				mem[addrb] <= dinb;
			end
			
			douta <= mem[addra];
			doutb <= mem[addrb];
		end
		else if(ena) begin
			if(wea) begin
				mem[addra] <= dina;
			end
			douta <= mem[addra];
		end
		else if (enb) begin
			if(web) begin
				mem[addrb] <= dinb;
			end
			doutb <= mem[addrb];
		end
		else begin
			douta <= 0;
			doutb <= 0;
		end
		
	end
	
endmodule
module Dual_Port_RAM_3
#(
  parameter AWIDTH = 13, // address width
  parameter DWIDTH = 32,
  parameter KWIDTH = 5,
  parameter JWIDTH = 3 // data width
)
(
  input clka, // clock
  ///*** Port A***///
  input ena, // port A read enable
  input wea, // port A write enable
  input [AWIDTH-1:0] addra, // port A address
  input [DWIDTH-1:0] dina, // port A data
  input enjka,
  input enjkb,
  output [DWIDTH-1-JWIDTH-KWIDTH:0] douta, // port A data output
  output [KWIDTH-1:0] kouta,
  output [JWIDTH-1:0] jouta,

  ///*** Port B***///
  input clkb, // clock
  input enb, // port A read enable
  input web, // port A write enable
  input [AWIDTH-1:0] addrb, // port A address
  input [DWIDTH-1:0] dinb, // port A data
  output [DWIDTH-1-JWIDTH-KWIDTH:0] doutb, // port A data output
  output [KWIDTH-1:0] koutb,
  output [JWIDTH-1:0] joutb
);
    reg [DWIDTH-1:0] dataa;
    reg [DWIDTH-1:0] datab;
    reg [KWIDTH-1:0] ka;
    reg [JWIDTH-1:0] ja;
    reg [KWIDTH-1:0] kb;
    reg [JWIDTH-1:0] jb;
    reg [DWIDTH-1:0] mem [2**AWIDTH-1:0];
    always @(posedge clka) begin
        // /*** Port A***///
        if (ena&enb) begin
            if(wea) begin
                mem[addra] <= dina;
            end
            else if(web) begin
                mem[addrb] <= dinb;
            end

            dataa <= mem[addra];
            

            datab <= mem[addrb];
            
        end
        else if(ena) begin
            if(wea) begin
                mem[addra] <= dina;
            end
            dataa <= mem[addra];
          
        end
        else if (enb) begin
            if(web) begin
                mem[addrb] <= dinb;
            end
            datab <= mem[addrb];
         
        end
//        if (enjka&enjkb) begin 
//            kouta <= dataa[KWIDTH+JWIDTH-1:JWIDTH];
//            jouta <= dataa[JWIDTH-1:0];
//            koutb <= datab[KWIDTH+JWIDTH-1:JWIDTH];
//            joutb <= datab[JWIDTH-1:0];
//        end
//        if (enjka) begin 
//            kouta <= dataa[KWIDTH+JWIDTH-1:JWIDTH];
//            jouta <= dataa[JWIDTH-1:0];
//        end
//        if (enjkb) begin 
//            koutb <= datab[KWIDTH+JWIDTH-1:JWIDTH];
//            joutb <= datab[JWIDTH-1:0];
//        end
        
    end
    assign douta = dataa[DWIDTH-1:KWIDTH+JWIDTH];
    assign kouta = dataa[KWIDTH+JWIDTH-1:JWIDTH];
    assign jouta = dataa[JWIDTH-1:0];
    assign koutb = datab[KWIDTH+JWIDTH-1:JWIDTH];
    assign joutb = datab[JWIDTH-1:0];
    assign doutb = datab[DWIDTH-1:KWIDTH+JWIDTH];
    
endmodule